/******************************************************************************
* ctcp.c
* ------
* Implementation of cTCP done here. This is the only file you need to change.
* Look at the following files for references and useful functions:
*   - ctcp.h: Headers for this file.
*   - ctcp_linked_list.h: Linked list functions for managing a linked list.
*   - ctcp_sys.h: Connection-related structs and functions, cTCP segment
*                 definition.
*   - ctcp_utils.h: Checksum computation, getting the current time.
*
*****************************************************************************/

#include "ctcp.h"
#include "ctcp_linked_list.h"
#include "ctcp_sys.h"
#include "ctcp_utils.h"
#include "ctcp_bbr.h"

/**
* Linked list of connection states. Go through this in ctcp_timer() to
* resubmit segments and tear down connections.
*/
static ctcp_state_t *state_list;
static FILE *fp;

ctcp_state_t *ctcp_init(conn_t *conn, ctcp_config_t *cfg) {
	/* Connection could not be established. */
	if (conn == NULL) {
		return NULL;
	}

	/* Established a connection. Create a new state and update the linked list
	of connection states. */
	ctcp_state_t *state = calloc(sizeof(ctcp_state_t), 1);
	state->next = state_list;
	state->prev = &state_list;
	if (state_list)
		state_list->prev = &state->next;
	state_list = state;

	/* Set fields. */
	state->conn = conn;
	/* FIXME: Do any other initialization here. */
	state->segments = ll_create();
	state->ackno = 1;
	state->seqno = 1;
	state->next_seqno = 1;
	state->recv_window = cfg->recv_window;
	state->send_window = cfg->send_window;
	state->rt_timeout = cfg->rt_timeout;
	state->read_finish = 0;
	state->output = NULL;
	state->output_len = 0;
	state->current_time = current_time();
	state->FIN_received = 0;
	state->FIN_sent = 0;
	state->get_all_ack = 0;
	state->all_recv_output = 0;
	state->retrans_count = 0;

	state->bbr = calloc(1, sizeof(struct ctcp_bbr));
	init_bbr(state->bbr);

	fp = fopen("bdp.txt", "w");
	// //fprintf(stderr, "ctcp_init finish !!!\n");
	return state;
}

void ctcp_destroy(ctcp_state_t *state) {
	/* Update linked list. */
	// //fprintf(stderr, "ctcp_destroy start...\n");
	if (state->next)
		state->next->prev = state->prev;

	*state->prev = state->next;
	conn_remove(state->conn);

	/* FIXME: Do any other cleanup here. */

	ll_destroy(state->segments);
	free(state);
	end_client();
	// //fprintf(stderr, "ctcp_destroy finish !!!\n");
}

void ctcp_read(ctcp_state_t *state) {
	/* FIXME */
	// //fprintf(stderr, "ctcp_read start...\n");
	if (state == NULL)
		return;
	if (state->read_finish == 1){
		return;
	}
	if (current_time() <= state->bbr->nextSendTime){
		////fprintf(stderr, "currtent_time = %ld, nextsendtime = %ld\n", current_time(), state->bbr->nextSendTime);
		////fprintf(stderr, "dont read now  !!!\n");
		return;
	}
	// flow control on sender side, segment inflight doesn't exceed sender window size
	if (state->next_seqno >= state->seqno + state->send_window)
		return;

	char buffer[MAX_SEG_DATA_SIZE];
	int input_res = conn_input(state->conn, buffer, MAX_SEG_DATA_SIZE);
	if (input_res == -1){	//error or EOF
		//fprintf(stderr, "read finished !!!\n");
		state->read_finish = 1;
		//ctcp_send_fin(state);
		//state->FIN_sent = 1;
	}
	else if (input_res == 0){	//no data
		return;
	}
	else{	//there are input_res bytes data
		//fprintf(stderr, "send DATA\n");
		// ctcp_send_data(state, buffer, input_res);
		bbr_send(state, state->bbr, buffer, input_res);
	}

	// //fprintf(stderr, "ctcp_read finish !!!\n");
}

void ctcp_receive(ctcp_state_t *state, ctcp_segment_t *segment, size_t len) {
	/* FIXME */
	// //fprintf(stderr, "ctcp_receive start...\n");
	// print_hdr_ctcp(segment);
	if (state == NULL)
		return;
	// check cksum, if checksum is wrong ask for retransmit
	//if (check_cksum(segment) == 0){
	////fprintf(stderr, "checksum wrong\n");
	//ctcp_send_ack(state);
	//}
	//else{
	int data_len = len - sizeof(ctcp_segment_t);
	// data segment
	if (data_len > 0){
		//fprintf(stderr, "receive DATA\n");
		// for segments already received, ignore
		if (ntohl(segment->seqno) < state->ackno)
			return;
		// for segments where data larger than receive window, ignore
		if (ntohl(segment->seqno) > state->ackno + state->recv_window)
			return;
		// for segments match ackno, receive and send ack back
		if (ntohl(segment->seqno) == state->ackno){
			// enough bufspace, output
			if (conn_bufspace(state->conn) >= data_len){
				state->output = calloc(1, data_len); // state->output is freed in ctcp_output
				state->output_len = data_len;
				memcpy(state->output, segment->data, data_len);

				state->ackno += data_len; // updata ackno
				ctcp_output(state);
				ctcp_send_ack(state);
			}
			// no enough bufspace, ask for retransmission
			else{
				ctcp_send_ack(state);
			}

		}
		// for segments don't mactch ackno, ask for retransmission
		else{
			ctcp_send_ack(state);
		}
	}
	// ack segment
	if (ntohl(segment->flags) & (ACK)){
		//fprintf(stderr, "receive ACK\n");
		// print_hdr_ctcp(segment);
		// already send FIN
		if (state->FIN_sent == 1){
			// receive ack for FIN
			if (ntohl(segment->ackno) == state->seqno + 1){
				state->FIN_received = 1;
				// receive an ack, remove the segment in retransmit queue
				if (state->segments != NULL){
					ll_node_t *node = ll_front(state->segments);
					free(node->object);
					ll_remove(state->segments, node);
					state->retrans_count = 0;
				}
				return;
			}
		}
		if (ll_length(state->segments) != 0){
			// //fprintf(stderr, "ll_length(state->segments) != 0\n");
			// get state->next_seqno, check whether a segment in retransmit waiting list is receieved
			ctcp_segment_t *sgm = (ctcp_segment_t *)ll_front(state->segments)->object;
			uint16_t data_len = ntohs(sgm->len) - sizeof(ctcp_segment_t);
			uint32_t expected_ackno = ntohl(sgm->seqno) + data_len;
			// print_hdr_ctcp(sgm);
			//fprintf(stderr, "sgm->seqno: %d, expected_ackno: %d,   received ackno: %d \n", ntohl(sgm->seqno), expected_ackno, ntohl(segment->ackno));
			// ackno matches, this segment is received, remove from waiting list, update retransmission count, seqno and sender window
			if (expected_ackno == ntohl(segment->ackno)){
				//fprintf(stderr, "exact ACK\n");
				state->bbr->inflight -= data_len;
				if (sgm->delivered_time == 0){

					/*sgm->sendtime = current_time();
					sgm->delivered = state->bbr->delivered;
					sgm->delivered_time = state->bbr->delivered_time;*/
				}
				bbr_onAck(state->bbr, sgm);

				ll_node_t *node = ll_front(state->segments);
				free(node->object);
				ll_remove(state->segments, node);
				state->retrans_count = 0;
				state->seqno = expected_ackno;
				state->send_window = ntohs(segment->window);
			}
			// ackno larger than expected, this segment is received before but ack is lost, so remove from waiting list
			else if (expected_ackno < ntohl(segment->ackno)){
				//fprintf(stderr, "larger ACK\n");
				ll_node_t *node = ll_front(state->segments);
				state->bbr->inflight -= data_len;
				free(node->object);
				ll_remove(state->segments, node);
				state->retrans_count = 0;
				state->seqno = expected_ackno;
			}
			// ackno smaller than expected, retransmit this segment
			else{
				//fprintf(stderr, "smaller ACK\n");
				conn_send(state->conn, sgm, ntohs(sgm->len));
			}
			//fprintf(stderr, "inflight = %ld\n", state->bbr->inflight);
		}
	}
	// FIN segment
	if (ntohl(segment->flags) & (FIN)){
		// //fprintf(stderr, "receive FIN\n");
		state->FIN_received = 1;
		// if all segments before FIN received, send ack of FIN back
		if (ntohl(segment->seqno) == state->ackno){
			// tread FIN as 1 byte for ack
			state->ackno += 1;
			ctcp_send_ack(state);
			conn_output(state->conn, 0, 0);

			state->all_recv_output = 1;
		}
	}
	//}

	// //fprintf(stderr, "ctcp_receive finish !!!\n");
}

void ctcp_output(ctcp_state_t *state) {
	/* FIXME */
	// //fprintf(stderr, "ctcp_output start...\n");
	int buf_space = conn_bufspace(state->conn);
	if (buf_space >= state->output_len){
		conn_output(state->conn, state->output, state->output_len);
		free(state->output);
		state->output = NULL;
		state->output_len = 0;
	}
	// //fprintf(stderr, "ctcp_output finish !!!\n");
}

void ctcp_timer() {
	/* FIXME */
	// // //fprintf(stderr, "ctcp_timer start...\n");
	if (state_list == NULL)
		return;
	ctcp_state_t *state = state_list;
	if ((state->read_finish == 1) && (state->get_all_ack == 1) && (state->FIN_sent == 0)){
		//fprintf(stderr, "send FIN\n");
		ctcp_send_fin(state);
		fclose(fp);
		state->FIN_sent = 1;
	}
	// destroy state
	if (state->FIN_received == 1 && state->FIN_sent == 1 && state->read_finish == 1 && state->get_all_ack == 1 && state->all_recv_output == 1){
		//fprintf(stderr, "all conditions achieved, destroy\n");
		ctcp_destroy(state);
		return;
	}
	// state->segments is empty, means all sent segments are acked
	if (ll_length(state->segments) == 0){
		state->get_all_ack = 1;
		return;
	}
	// timeout and retransmit
	if (current_time() - state->current_time > state->rt_timeout){
		if (state->retrans_count <= RETRANSMIT_LIMIT){
			state->retrans_count++;
			state->current_time = current_time();
			ctcp_segment_t *sgm = (ctcp_segment_t *)ll_front(state->segments)->object;

			sgm->sendtime = current_time();
			sgm->delivered = state->bbr->delivered;
			sgm->delivered_time = state->bbr->delivered_time;

			conn_send(state->conn, sgm, ntohs(sgm->len));
			long bdp, now;
			now = current_time();
			bdp = state->bbr->max_btlbw * state->bbr->min_rtt_us;
			fprintf(fp, "%ld %ld\n", now, bdp * 8);
			fprintf(stderr, "write bdp.log, state->bbr->max_btlbw  = %f, state->bbr->min_rtt_us = %ld\n",
				state->bbr->max_btlbw, state->bbr->min_rtt_us);
			//ctcp_send_data(state, sgm->data, ntohs(sgm->len), current_time(), state->bbr->delivered, state->bbr->delivered_time);
			state->bbr->inflight += ntohs(sgm->len);
		}
		else{
			//fprintf(stderr, "retransmission destroy\n");
			ctcp_destroy(state);
		}
	}


	// // //fprintf(stderr, "ctcp_timer finish !!!\n");
}


/* FIXME: Feel free to add as many helper functions as needed. Don't repeat
code! Helper functions make the code clearer and cleaner. */

//**************helper functions*************************

// send fin segment
void ctcp_send_fin(ctcp_state_t *state){
	//fprintf(stderr, "ctcp_send_fin\n");
	//	init segment
	ctcp_segment_t * fin_seg = calloc(1, sizeof(ctcp_segment_t));
	fin_seg->ackno = htonl(state->ackno);
	fin_seg->seqno = htonl(state->next_seqno);
	fin_seg->flags = fin_seg->flags | htonl(FIN);
	fin_seg->len = htons(sizeof(ctcp_segment_t));
	fin_seg->window = htons(state->recv_window);
	fin_seg->cksum = htons(0);
	fin_seg->cksum = cksum(fin_seg, ntohs(fin_seg->len));
	fin_seg->delivered_time = current_time();

	// add in case retransmit
	ll_add(state->segments, fin_seg);
	state->get_all_ack = 0;
	// print_hdr_ctcp(fin_seg);
	conn_send(state->conn, fin_seg, sizeof(ctcp_segment_t));
}

// send data segment
void ctcp_send_data(ctcp_state_t *state, char* data, int data_bytes, long sendtime, long delivered, long delivered_time){
	//fprintf(stderr, "ctcp_send_data\n");
	state->bbr->inflight += data_bytes;
	//fprintf(stderr, "inflight = %ld\n", state->bbr->inflight);
	long bdp, now;
	now = current_time();
	bdp = state->bbr->max_btlbw * state->bbr->min_rtt_us;
	fprintf(fp, "%ld %ld\n", now, bdp * 8);
	fprintf(stderr, "write bdp.log, state->bbr->max_btlbw  = %f, state->bbr->min_rtt_us = %ld\n",
	state->bbr->max_btlbw, state->bbr->min_rtt_us);
	//	init segment
	ctcp_segment_t * data_seg = calloc(1, sizeof(ctcp_segment_t)+MAX_SEG_DATA_SIZE);
	data_seg->ackno = htonl(state->ackno);
	data_seg->seqno = htonl(state->next_seqno);
	data_seg->flags = data_seg->flags | htonl(ACK);
	memcpy(data_seg->data, data, data_bytes);
	data_seg->len = htons(sizeof(ctcp_segment_t)+data_bytes);
	data_seg->window = htons(state->send_window);
	data_seg->cksum = htons(0);
	data_seg->cksum = cksum(data_seg, ntohs(data_seg->len));
	data_seg->sendtime = sendtime;
	data_seg->delivered = delivered;
	data_seg->delivered_time = delivered_time;

	state->next_seqno = state->next_seqno + data_bytes;

	// add in case retransmit
	ll_add(state->segments, data_seg);
	state->get_all_ack = 0;
	// print_hdr_ctcp(data_seg);
	conn_send(state->conn, data_seg, ntohs(data_seg->len));
}

// send ack segment
void ctcp_send_ack(ctcp_state_t *state){
	//fprintf(stderr, "ctcp_send_ack\n");
	ctcp_segment_t * ack_seg = calloc(1, sizeof(ctcp_segment_t));
	ack_seg->ackno = htonl(state->ackno);
	ack_seg->seqno = htonl(state->next_seqno);
	ack_seg->flags = ack_seg->flags | htonl(ACK);
	ack_seg->len = htons(sizeof(ctcp_segment_t));
	ack_seg->window = htons(state->recv_window);
	ack_seg->cksum = htons(0);
	ack_seg->cksum = cksum(ack_seg, ntohs(ack_seg->len));

	// print_hdr_ctcp(ack_seg);
	conn_send(state->conn, ack_seg, ntohs(ack_seg->len));
	free(ack_seg);
}

// check cksum
int check_cksum(ctcp_segment_t *sgm){
	uint16_t pre_cksum = ntohs(sgm->cksum);
	sgm->cksum = htons(0);
	sgm->cksum = cksum(sgm, ntohs(sgm->len));
	if (pre_cksum == ntohs(sgm->cksum)){
		// // //fprintf(stderr, "check_cksum correct\n");
		return 1;
	}
	else{
		// // //fprintf(stderr, "check_cksum wrong\n");
		return 0;
	}
}

//---------------------BBR---------------------------------------


//***************helper functions end*************************

