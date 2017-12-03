#include "ctcp_bbr.h"
#include "ctcp.h"
#include "ctcp_utils.h"
#include "ctcp_linked_list.h"

static float ProbeBW_pacing[8] = { 5 / 4, 3 / 4, 1, 1, 1, 1, 1, 1 };
//init bbr
void init_bbr(struct ctcp_bbr *bbr){
	bbr->mode = BBR_STARTUP;
	bbr->deliveryRate = 0;
	bbr->pacing_gain = 2885 / 1000;
	bbr->cwnd_gain = 0.0;
	bbr->delivered = 0;
	bbr->delivered_time = current_time();
	bbr->deliveryRate = 0;
	bbr->max_btlbw = 0.0;
	bbr->max_btlbw_last = 0.0;
	bbr->inflight = 0;
	bbr->nextSendTime = 0;
	bbr->btlbw_not_incre_cnt = 0;
	bbr->ProbeBW_index = 0;

	int i = 0;
	for (; i < FILTER_SIZE; i++){
		bbr->RTpropFilter[i] = 0x7fffffff;
	}

}

void ctcp_bbr_startup(struct ctcp_bbr *bbr){
	//fprintf(stderr, "STARTUP\n");
	bbr->pacing_gain = 2885 / 1000; // 2 / ln(2)
	bbr->cwnd_gain = 2885 / 1000;
}
void ctcp_bbr_drain(struct ctcp_bbr *bbr){
	//fprintf(stderr, "DRAIN\n");
	bbr->pacing_gain = 1000 / 2885;
}
void ctcp_bbr_probe_bw(struct ctcp_bbr *bbr){
	//fprintf(stderr, "PROBEBW\n");
	bbr->ProbeBW_index %= 8;
	bbr->pacing_gain = ProbeBW_pacing[bbr->ProbeBW_index++];
}
void ctcp_bbr_probe_rtt(struct ctcp_bbr *bbr){
	//fprintf(stderr, "PROBERTT\n");

}
void bbr_onAck(struct ctcp_bbr *bbr, ctcp_segment_t *sgm){
	//fprintf(stderr, "bbr_onAck\n");

	// check state when received an ACK

	//bbr state machine
	switch (bbr->mode)
	{
	case BBR_STARTUP:
		ctcp_bbr_startup(bbr);

		if (bbr->max_btlbw_last * 1.25 > bbr->max_btlbw){
			bbr->btlbw_not_incre_cnt++;
		}
		if (bbr->btlbw_not_incre_cnt == 3){
			bbr->mode = BBR_DRAIN;
			bbr->btlbw_not_incre_cnt = 0;
			ctcp_bbr_drain(bbr);
		}
		break;
	case BBR_DRAIN:
		ctcp_bbr_drain(bbr);
		if (bbr->inflight <= bbr->max_btlbw * bbr->min_rtt_us){
			bbr->mode = BBR_PROBE_BW;
			bbr->ProbeBW_index = 0;
			ctcp_bbr_probe_bw(bbr);
		}
		break;
	case BBR_PROBE_BW:
		ctcp_bbr_probe_bw(bbr);
		break;
	case BBR_PROBE_RTT:
		ctcp_bbr_probe_rtt(bbr);
		break;
	default:
		break;
	}


	long rtt, pkt_size, cur_time;
	cur_time = current_time();
	//rtt = cur_time - sgm->sendtime;
	rtt = cur_time - sgm->delivered_time;
	////fprintf(stderr, "current time = %ld, sgm->sendtime = %ld\n",
	//	cur_time, sgm->sendtime);
	fprintf(stderr, "current time = %ld, sgm->delivered_time = %ld\n",
	cur_time, sgm->delivered_time);
	fprintf(stderr, "rtt is : %ld\n", rtt);

	update_min_filter(bbr, rtt);
	pkt_size = ntohs(sgm->len) - sizeof(ctcp_segment_t);
	bbr->delivered += pkt_size;
	bbr->delivered_time = cur_time;
	bbr->deliveryRate = (float)(bbr->delivered - sgm->delivered) / (float)(cur_time - sgm->delivered_time);
	//fprintf(stderr, "current time = %ld, sgm->delivered_time = %ld\n", cur_time, sgm->delivered_time);
	//fprintf(stderr, "bbr->delivered - sgm->delivered = %ld, cur_time - sgm->delivered_time = %ld\n", 
	//bbr->delivered - sgm->delivered, cur_time - sgm->delivered_time);

	bbr->max_btlbw_last = bbr->max_btlbw;
	//fprintf(stderr, "deliveryRate is : %f\n", bbr->deliveryRate);
	//fprintf(stderr, "max_btlbw is : %f\n", bbr->max_btlbw);
	if (bbr->deliveryRate > bbr->max_btlbw){ // ignore app_limited
		update_max_filter(bbr, bbr->deliveryRate);
		//fprintf(stderr, "max_btlbw is : %f\n", bbr->max_btlbw);
	}

}
void bbr_send(ctcp_state_t *state, struct ctcp_bbr *bbr, char* data, int data_bytes){
	//fprintf(stderr, "bbr_send\n");
	long bdp, now;
	now = current_time();
	bdp = bbr->max_btlbw * bbr->min_rtt_us;
	//fprintf(stderr, "inflight is : %ld\n", bbr->inflight);
	//fprintf(stderr, "bdp is : %ld\n", bdp);
	if (bbr->inflight > bbr->cwnd_gain * bdp){
		ctcp_segment_t * data_seg = calloc(1, sizeof(ctcp_segment_t)+MAX_SEG_DATA_SIZE);
		data_seg->ackno = htonl(state->ackno);
		data_seg->seqno = htonl(state->next_seqno);
		data_seg->flags = data_seg->flags | htonl(ACK);
		memcpy(data_seg->data, data, data_bytes);
		data_seg->len = htons(sizeof(ctcp_segment_t)+data_bytes);
		data_seg->window = htons(state->send_window);
		data_seg->cksum = htons(0);
		data_seg->cksum = cksum(data_seg, ntohs(data_seg->len));
		data_seg->delivered_time = state->bbr->delivered_time;

		state->next_seqno = state->next_seqno + data_bytes;

		// add in case retransmit
		ll_add(state->segments, data_seg);
		state->get_all_ack = 0;


		return;
	}
	if (now > bbr->nextSendTime){
		//fprintf(stderr, "now > bbr->nextSendTime !!!, bbr->nextSendTime = %ld\n", bbr->nextSendTime);
		ctcp_send_data(state, data, data_bytes, now, bbr->delivered, bbr->delivered_time);
		if (bbr->pacing_gain * bbr->max_btlbw == 0){
			bbr->nextSendTime = now;
		}
		else{
			bbr->nextSendTime = now + data_bytes / (bbr->pacing_gain * bbr->max_btlbw);
		}
		//fprintf(stderr, "---------------------------now = %ld, bbr->nextSendTime = %ld, bbr->pacing_gain * bbr->max_btlbw = %f\n", 
		//now, bbr->nextSendTime, bbr->pacing_gain * bbr->max_btlbw);
	}
	//fprintf(stderr, "bbr_send() finish !!!\n");
}

void update_min_filter(struct ctcp_bbr *bbr, long rtt){
	fprintf(stderr, "update_min_filter !!!\n");
	int i;
	bbr->min_rtt_us = rtt;
	fprintf(stderr, "rtt = %ld\n", rtt);
	for (i = 0; i < FILTER_SIZE - 1; i++){
		bbr->RTpropFilter[i] = bbr->RTpropFilter[i + 1];
		if (bbr->min_rtt_us > bbr->RTpropFilter[i])
			bbr->min_rtt_us = bbr->RTpropFilter[i];
	}
	bbr->RTpropFilter[i] = rtt;
	bbr->min_rtt_stamp = current_time();
	fprintf(stderr, "min_rtt = %ld\n", bbr->min_rtt_us);
}
void update_max_filter(struct ctcp_bbr *bbr, double rate){
	//fprintf(stderr, "update_max_filter !!!\n");
	int i;
	bbr->max_btlbw = rate;
	for (i = 0; i < FILTER_SIZE - 1; i++){
		bbr->BtlBwFilter[i] = bbr->BtlBwFilter[i + 1];
		if (bbr->max_btlbw < bbr->BtlBwFilter[i])
			bbr->max_btlbw = bbr->BtlBwFilter[i];
	}
	bbr->BtlBwFilter[i] = rate;
	bbr->max_btlbw_stamp = current_time();
}















