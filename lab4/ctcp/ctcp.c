/******************************************************************************
* ctcp.c
* ------
* Implementation of cTCP done here. This is the only file you need to change.
* Look at the following files for references and useful functions:
*   - ctcp.h: Headers for this file.
*   - ctcp_iinked_list.h: Linked list functions for managing a linked list.
*   - ctcp_sys.h: Connection-related structs and functions, cTCP segment
*                 definition.
*   - ctcp_utils.h: Checksum computation, getting the current time.
*
*****************************************************************************/

#include "ctcp.h"
#include "ctcp_linked_list.h"
#include "ctcp_sys.h"
#include "ctcp_utils.h"

/**
* Connection state.
*
* Stores per-connection information such as the current sequence number,
* unacknowledged packets, etc.
*
* You should add to this to store other fields you might need.
*/
struct ctcp_state {
	struct ctcp_state *next;  /* Next in linked list */
	struct ctcp_state **prev; /* Prev in linked list */

	conn_t *conn;             /* Connection object -- needed in order to figure
							  out destination when sending */
	linked_list_t *segments;  /* Linked list of segments sent to this connection.
							  It may be useful to have multiple linked lists
							  for unacknowledged segments, segments that
							  haven't been sent, etc. Lab 1 uses the
							  stop-and-wait protocol and therefore does not
							  necessarily need a linked list. You may remove
							  this if this is the case for you */

	/* FIXME: Add other needed fields. */
	uint32_t seqno;              /* Current sequence number */
	uint32_t next_seqno;         /* Sequence number of next segment to send */
	uint32_t ackno;              /* Current ack number */
	uint16_t recv_window;    /* Receive window size (in multiples of
							 MAX_SEG_DATA_SIZE) of THIS host. For Lab 1 this
							 value will be 1 * MAX_SEG_DATA_SIZE */
	uint16_t send_window;    /* Send window size (a.k.a. receive window size of
							 the OTHER host). For Lab 1 this value
							 will be 1 * MAX_SEG_DATA_SIZE */
	int rt_timeout;          /* Retransmission timeout, in ms */
	int read_finish;		/* mark whether read is finished */
	char *output;			/* bytes to output, for ctcp_output() */
	int output_len;			/* length of output */
	long current_time;		/* record current time */
	int FIN_received;		/* already received FIN from sender, sender disconnected */
	int FIN_sent;			/* already sent FIN, disconnected to receiver*/
	int get_all_ack;		/* all sent segments are acked */
	int all_recv_output;	/* outpute all received segments */
	int retrans_count;		/* count for retransmit times */

};

/**
* Linked list of connection states. Go through this in ctcp_timer() to
* resubmit segments and tear down connections.
*/
static ctcp_state_t *state_list;

/* FIXME: Feel free to add as many helper functions as needed. Don't repeat
code! Helper functions make the code clearer and cleaner. */

//**************helper functions*************************

// send fin segment
void ctcp_send_fin(ctcp_state_t *state){
	// fprintf(stderr, "ctcp_send_fin\n");
	//	init segment
	ctcp_segment_t * fin_seg = calloc(1, sizeof(ctcp_segment_t));
	fin_seg->ackno = htonl(state->ackno);
	fin_seg->seqno = htonl(state->next_seqno);
	fin_seg->flags = fin_seg->flags | htonl(FIN);
	fin_seg->len = htons(sizeof(ctcp_segment_t));
	fin_seg->window = htons(state->recv_window);
	fin_seg->cksum = htons(0);
	fin_seg->cksum = cksum(fin_seg, ntohs(fin_seg->len));

	// add in case retransmit
	ll_add(state->segments, fin_seg);
	state->get_all_ack = 0;
	// print_hdr_ctcp(fin_seg);
	conn_send(state->conn, fin_seg, sizeof(ctcp_segment_t));
}

// send data segment
void ctcp_send_data(ctcp_state_t *state, char* data, int data_bytes){
	// fprintf(stderr, "ctcp_send_data\n");
	//	init segment
	ctcp_segment_t * data_seg = calloc(1, sizeof(ctcp_segment_t) + MAX_SEG_DATA_SIZE);
	data_seg->ackno = htonl(state->ackno);
	data_seg->seqno = htonl(state->next_seqno);
	data_seg->flags = data_seg->flags | htonl(ACK);
	memcpy(data_seg->data, data, data_bytes);
	data_seg->len = htons(sizeof(ctcp_segment_t)+data_bytes);
	data_seg->window = htons(state->send_window);
	data_seg->cksum = htons(0);
	data_seg->cksum = cksum(data_seg, ntohs(data_seg->len));

	state->next_seqno = state->next_seqno + data_bytes;

	// add in case retransmit
	ll_add(state->segments, data_seg);
	state->get_all_ack = 0;
	// print_hdr_ctcp(data_seg);
	conn_send(state->conn, data_seg, ntohs(data_seg->len));
}

// send ack segment
void ctcp_send_ack(ctcp_state_t *state){
	// fprintf(stderr, "ctcp_send_ack\n");
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
		// // fprintf(stderr, "check_cksum correct\n");
		return 1;
	}
	else{
		// // fprintf(stderr, "check_cksum wrong\n");
		return 0;
	}
}
//***************helper functions end*************************

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

	// fprintf(stderr, "ctcp_init finish !!!\n");
	return state;
}

void ctcp_destroy(ctcp_state_t *state) {
	/* Update linked list. */
	// fprintf(stderr, "ctcp_destroy start...\n");
	if (state->next)
		state->next->prev = state->prev;

	*state->prev = state->next;
	conn_remove(state->conn);

	/* FIXME: Do any other cleanup here. */

	ll_destroy(state->segments);
	free(state);
	end_client();
	// fprintf(stderr, "ctcp_destroy finish !!!\n");
}

void ctcp_read(ctcp_state_t *state) {
	/* FIXME */
	// fprintf(stderr, "ctcp_read start...\n");
	if (state == NULL)
		return;
	if (state->read_finish == 1){
		return;
	}
	// flow control on sender side, segment inflight doesn't exceed sender window size
	if (state->next_seqno >= state->seqno + state->send_window)
		return;

	char buffer[MAX_SEG_DATA_SIZE];
	int input_res = conn_input(state->conn, buffer, MAX_SEG_DATA_SIZE);
	if (input_res == -1){	//error or EOF
		// fprintf(stderr, "send FIN\n");
		state->read_finish = 1;
		ctcp_send_fin(state);
		state->FIN_sent = 1;
	}
	else if (input_res == 0){	//no data
		return;
	}
	else{	//there are input_res bytes data
		// fprintf(stderr, "send DATA\n");
		ctcp_send_data(state, buffer, input_res);
	}

	// fprintf(stderr, "ctcp_read finish !!!\n");
}

void ctcp_receive(ctcp_state_t *state, ctcp_segment_t *segment, size_t len) {
	/* FIXME */
	// fprintf(stderr, "ctcp_receive start...\n");
	// print_hdr_ctcp(segment);
	if (state == NULL)
		return;
	// check cksum, if checksum is wrong ask for retransmit
	if (check_cksum(segment) == 0){
		ctcp_send_ack(state);
	}
	else{
		int data_len = len - sizeof(ctcp_segment_t);
		// data segment
		if (data_len > 0){
			// fprintf(stderr, "receive DATA\n");
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
			// fprintf(stderr, "receive ACK\n");
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
				// fprintf(stderr, "ll_length(state->segments) != 0\n");
				// get state->next_seqno, check whether a segment in retransmit waiting list is receieved
				ctcp_segment_t *sgm = (ctcp_segment_t *)ll_front(state->segments)->object;
				uint16_t data_len = ntohs(sgm->len) - sizeof(ctcp_segment_t);
				uint32_t expected_ackno = ntohl(sgm->seqno) + data_len;
				// print_hdr_ctcp(sgm);
				// fprintf(stderr, "sgm->seqno: %d, expected_ackno: %d,   received ackno: %d \n", ntohl(sgm->seqno), expected_ackno, ntohl(segment->ackno));
				// ackno matches, this segment is received, remove from waiting list, update retransmission count, seqno and sender window
				if (expected_ackno == ntohl(segment->ackno)){
					ll_node_t *node = ll_front(state->segments);
					free(node->object);
					ll_remove(state->segments, node);
					state->retrans_count = 0;
					state->seqno = expected_ackno;
					state->send_window = ntohs(segment->window);
				}
				// ackno larger than expected, this segment is received before but ack is lost, so remove from waiting list
				else if (expected_ackno < ntohl(segment->ackno)){
					ll_node_t *node = ll_front(state->segments);
					free(node->object);
					ll_remove(state->segments, node);
					state->retrans_count = 0;
					state->seqno = expected_ackno;
				}
				// ackno smaller than expected, retransmit this segment
				else{
					conn_send(state->conn, sgm, ntohs(sgm->len));
				}
			}
		}
		// FIN segment
		if (ntohl(segment->flags) & (FIN)){
			// fprintf(stderr, "receive FIN\n");
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
	}

	// fprintf(stderr, "ctcp_receive finish !!!\n");
}

void ctcp_output(ctcp_state_t *state) {
	/* FIXME */
	// fprintf(stderr, "ctcp_output start...\n");
	int buf_space = conn_bufspace(state->conn);
	if (buf_space >= state->output_len){
		conn_output(state->conn, state->output, state->output_len);
		free(state->output);
		state->output = NULL;
		state->output_len = 0;
	}
	// fprintf(stderr, "ctcp_output finish !!!\n");
}

void ctcp_timer() {
	/* FIXME */
	// // fprintf(stderr, "ctcp_timer start...\n");
	if (state_list == NULL)
		return;

	ctcp_state_t *state = state_list;
	// destroy state
	if (state->FIN_received == 1 && state->FIN_sent == 1 && state->read_finish == 1 && state->get_all_ack == 1 && state->all_recv_output == 1){
		// fprintf(stderr, "all conditions achieved, destroy\n");
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
			conn_send(state->conn, sgm, ntohs(sgm->len));
		}
		else{
			// fprintf(stderr, "retransmission destroy\n");
			ctcp_destroy(state);
		}
	}

	// // fprintf(stderr, "ctcp_timer finish !!!\n");
}

