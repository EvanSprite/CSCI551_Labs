#include "ctcp_sys.h"
#include "ctcp.h"

#define BTLBW_NOT_INCRE_LIMIT 3

/* BBR has the following modes for deciding how fast to send: */
enum bbr_mode {
	BBR_STARTUP,	/* ramp up sending rate rapidly to fill pipe */
	BBR_DRAIN,	/* drain any queue created during startup */
	BBR_PROBE_BW,	/* discover, share bw: pace around estimated bw */
	BBR_PROBE_RTT,	/* cut cwnd to min to probe min_rtt */

};

/* BBR congestion control block */
struct ctcp_bbr {
	//uint32_t	probe_rtt_done_stamp;   /* end time for BBR_PROBE_RTT mode */
	int mode;		     /* current bbr_mode in state machine */
	float	pacing_gain;	/* current gain for setting pacing rate */
	float	cwnd_gain;	/* current gain for setting cwnd */
	long RTpropFilter[FILTER_SIZE]; /* min_rtt filter, an array, keeps rtts for select min*/
	long	min_rtt_us;	        /* min RTT in min_rtt_win_sec window */
	long	min_rtt_stamp;	        /* timestamp of min_rtt_us */
	long delivered;
	long delivered_time;
	double deliveryRate;
	long BtlBwFilter[FILTER_SIZE]; /* max_BtlBw filter, an array, keeps bandwidth for select max*/
	double	max_btlbw;	        /* max BtlBw  */
	long	max_btlbw_stamp;	        /* timestamp of max_btlbw_us */
	int btlbw_not_incre_cnt;	/* count of btlbw not increase, if reach BTLBW_NOT_INCRE_LIMIT, jump to drain*/
	double max_btlbw_last;		/* last max BtlBw*/

	long inflight;
	long nextSendTime;
	int ProbeBW_index;			/* pacing gain index for ProbeBW state*/
};

//init bbr
void init_bbr(struct ctcp_bbr *bbr);

void ctcp_bbr_startup(struct ctcp_bbr *bbr);
void ctcp_bbr_drain(struct ctcp_bbr *bbr);
void ctcp_bbr_probe_bw(struct ctcp_bbr *bbr);
void ctcp_bbr_probe_rtt(struct ctcp_bbr *bbr);
void bbr_onAck(struct ctcp_bbr *bbr, ctcp_segment_t *sgm);
void update_min_filter();
void bbr_send(ctcp_state_t *state, struct ctcp_bbr *bbr, char* data, int data_bytes);
void update_max_filter(struct ctcp_bbr *bbr, double rate);











