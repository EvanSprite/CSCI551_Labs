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

struct Q
{
	uint32_t seq;
	int size;
	char data[MAX_SEG_DATA_SIZE];
};
typedef struct Q Q_t;
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
	int win_size;
	int timer;     // 1/5 timeout
	int timeout;


	int EOF_from_peer;        //handle EOF
	int read_EOF_from_input;  //handle EOF
	int all_acked;            //handle EOF
	int timeout_rounds;       //handle EOF


	uint32_t next_pkt_to_send;  //upper edge of sender's window + 1
	uint32_t ack_expected;
	Q_t out_q[MAX_SEG_WIN_SIZE];
	int ack_timer[MAX_SEG_WIN_SIZE];
	int acked[MAX_SEG_WIN_SIZE];
	int num_buffered;           //how many output buffers currently used

	uint32_t pkt_expected;
	uint32_t largest_accpeptable_pkt; //upper edge of receiver's edge + 1
	Q_t in_q[MAX_SEG_WIN_SIZE];
	int arrived[MAX_SEG_WIN_SIZE];        //inbound bit map

};

/**
* Linked list of connection states. Go through this in ctcp_timer() to
* resubmit segments and tear down connections.
*/
static ctcp_state_t *state_list;

/* FIXME: Feel free to add as many helper functions as needed. Don't repeat
code! Helper functions make the code clearer and cleaner. */
//--------------helpler--------start-------------------------------

void send_sgm(ctcp_state_t *state, int pkt_kind, uint32_t seq){
	//fprintf(stderr, "%d, call send_sgm\n", getpid());

	ctcp_segment_t pkt;
	memset(pkt.data, '\0', MAX_SEG_DATA_SIZE);
	int index = seq % state->win_size;
	if (pkt_kind == DATA_PKT){
		pkt.len = htons(DATA_HEADER_LEN + state->out_q[index].size);
		pkt.seqno = htonl(seq);
		pkt.ackno = htonl(state->pkt_expected);
		memcpy(pkt.data, state->out_q[index].data, state->out_q[index].size);
		pkt.cksum = 0;
		pkt.cksum = cksum(&pkt, ntohs(pkt.len));
		state->ack_timer[index] = 0;
		state->acked[index] = 0;
		//fprintf(stderr, "send_sgm-->DATA_PKT\n");
		//print_hdr_ctcp(&pkt);
	}
	else if (pkt_kind == ACK_PKT){
		pkt.len = htons(ACK_HEADER_LEN);
		pkt.seqno = htonl(seq);
		pkt.ackno = htonl(state->pkt_expected);
		pkt.cksum = 0;
		pkt.cksum = cksum(&pkt, ntohs(pkt.len));
		//fprintf(stderr, "send_sgm-->ACK_PKT\n");
		print_hdr_ctcp(&pkt);
	}
	else{ //EOF_PKT
		pkt.len = htons(DATA_HEADER_LEN);
		pkt.seqno = htonl(seq);
		pkt.ackno = htonl(state->pkt_expected);
		memset(pkt.data, '\0', MAX_SEG_DATA_SIZE);
		pkt.cksum = 0;
		pkt.cksum = cksum(&pkt, DATA_HEADER_LEN);
		state->ack_timer[index] = 0;
		state->acked[index] = 0;
		//fprintf(stderr, "send_sgm-->EOF_PKT\n");
		print_hdr_ctcp(&pkt);
	}
	//fprintf(stderr, "send_sgm-->conn_send called\n");
	conn_send(state->conn, &pkt, ntohs(pkt.len));
	//fprintf(stderr, "send_sgm finished!!!\n");
}

int check_cksum(ctcp_segment_t *pkt, size_t len){
	uint16_t cks = pkt->cksum;
	pkt->cksum = 0;
	pkt->cksum = cksum(pkt, len);
	if (cks == pkt->cksum){
		return 1;
	}
	else{
		return 0;
	}
}

int is_all_acked(ctcp_state_t *state){
	int i, index;
	for (i = state->ack_expected; i < state->next_pkt_to_send; i++){
		index = i % state->win_size;
		if (state->acked[index] == 0){
			break;
		}
	}
	if (i == state->next_pkt_to_send){
		return 1;
	}
	else{
		return 0;
	}
}





//--------------helpler--------end-------------------------------

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

	state->win_size = cfg->recv_window;
	state->timer = cfg->timer;
	state->timeout = cfg->rt_timeout;

	state->next_pkt_to_send = 1;
	state->ack_expected = 1;
	state->num_buffered = 0;
	state->EOF_from_peer = 0;
	state->read_EOF_from_input = 0;
	state->all_acked = 0;

	state->pkt_expected = 1;
	state->largest_accpeptable_pkt = state->win_size + 1;
	int i;
	for (i = 0; i < state->win_size; i++){
		state->acked[i] = 0;
		state->arrived[i] = 0;
	}
	//free(cfg);
	//fprintf(stderr, "%d, ctcp_init finished!!!\n", getpid());
	return state;
}

void ctcp_destroy(ctcp_state_t *state) {
	//fprintf(stderr, "%d, call ctcp_destroy\n", getpid());
	/* Update linked list. */
	if (state->next)
		state->next->prev = state->prev;

	*state->prev = state->next;
	conn_remove(state->conn);

	/* FIXME: Do any other cleanup here. */

	free(state);
	end_client();
	//fprintf(stderr, "ctcp_destroy finish!!!\n");
}

void ctcp_read(ctcp_state_t *state) {
	/* FIXME */
	//fprintf(stderr, "%d, call ctcp_read\n", getpid());
	while (state->num_buffered < state->win_size && !state->read_EOF_from_input){
		int index = state->next_pkt_to_send % state->win_size;
		memset(state->out_q[index].data, '\0', MAX_SEG_DATA_SIZE);
		int ci = conn_input(state->conn, state->out_q[index].data, MAX_SEG_DATA_SIZE);
		//fprintf(stderr, "%d, Read Length = %d\n", getpid(), ci);
		if (ci > 0){ //there is input from STDIN
			//fprintf(stderr, "%d, Read Data num_buffered = %d\n", getpid(), state->num_buffered);
			state->num_buffered++;
			state->out_q[index].seq = state->next_pkt_to_send;
			state->out_q[index].size = ci;
			send_sgm(state, DATA_PKT, state->next_pkt_to_send);
			state->next_pkt_to_send++;
			//fprintf(stderr, "testnig------------------");
		}
		else if (ci < 0){ //EOF or error
			//fprintf(stderr, "%d, Read EOF num_buffered = %d\n", getpid(), state->num_buffered);
			state->num_buffered++;
			send_sgm(state, EOF_PKT, state->next_pkt_to_send);
			state->read_EOF_from_input = 1;
			state->next_pkt_to_send++;
		}
		else{ //no data to read
			//fprintf(stderr, "%d, ctcp_read-->no data\n", getpid());
			return;
		}
		//fprintf(stderr, "ctcp_read-->while\n");
	}
	//fprintf(stderr, "%d, ctcp_read finish!!!\n", getpid());
	return;
}

void ctcp_receive(ctcp_state_t *state, ctcp_segment_t *segment, size_t len) {
	/* FIXME */
	//fprintf(stderr, "%d, call ctcp_receive\n", getpid());
	if (len < ACK_HEADER_LEN || (len > ACK_HEADER_LEN && len < DATA_HEADER_LEN)){ //wrong length
		send_sgm(state, ACK_PKT, 0);
		return;
	}
	else if (check_cksum(segment, len) == 0){ //wrong checksum
		send_sgm(state, ACK_PKT, 0);
		return;
	}
	else{
		if (len == ACK_HEADER_LEN){
			//fprintf(stderr, "receive ACK\n");
		}
		else if (len == DATA_HEADER_LEN){
			//fprintf(stderr, "receive EOF\n");
		}
		else{
			//fprintf(stderr, "receive DATA\n");
		}
		print_hdr_ctcp(segment);

		// handle ackno
		//fprintf(stderr, "ctcp_receive()->handle ACKno\n");
		while (state->ack_expected <= ntohl(segment->ackno) - 1 && ntohl(segment->ackno) - 1 < state->next_pkt_to_send){
			int index = (state->ack_expected) % state->win_size;
			state->ack_timer[index] = 0;
			state->acked[index] = 1;
			state->num_buffered--;
			state->ack_expected++;
			state->acked[(state->ack_expected) % state->win_size] = 0;
			state->all_acked = is_all_acked(state);
			//fprintf(stderr, "%d handle ack-->EOF_from_peer = %d,  read_EOF_from_input = %d,  all_acked = %d\n", getpid(), state->EOF_from_peer, state->read_EOF_from_input, state->all_acked);
			if (state->EOF_from_peer == 1 && state->read_EOF_from_input == 1 && state->all_acked == 1){
				ctcp_destroy(state);
				//fprintf(stderr, "%d, ctcp_receive-->ctcp_destroy finish!!!\n", getpid());
			}
		}
		//fprintf(stderr, "%d,  sender  ack_expected = %d,   segment->ackno = %d,    next_pkt_to_send = %d\n", getpid(), state->ack_expected, ntohl(segment->ackno), state->next_pkt_to_send);

		// handle seqno
		//fprintf(stderr, "ctcp_receive()->handle SEQno\n");
		if (len >= DATA_HEADER_LEN){
			if (ntohl(segment->seqno) != state->pkt_expected){
				send_sgm(state, ACK_PKT, 0);
			}
			int index = ntohl(segment->seqno) % state->win_size;
			if (state->pkt_expected <= ntohl(segment->seqno) && ntohl(segment->seqno) < state->largest_accpeptable_pkt && !state->arrived[index]){
				state->in_q[index].seq = ntohl(segment->seqno);
				state->in_q[index].size = len - DATA_HEADER_LEN;
				memcpy(state->in_q[index].data, segment->data, len - DATA_HEADER_LEN);
				state->arrived[index] = 1;
				ctcp_output(state);
			}
		}
		//fprintf(stderr, "%d,  reciever  pkt_expected = %d,   segment->seqno = %d,   largest_accpeptable_pkt = %d\n", getpid(), state->pkt_expected, ntohl(segment->seqno), state->largest_accpeptable_pkt);

		if (state->num_buffered < state->win_size && state->read_EOF_from_input == 0){
			ctcp_read(state);
		}
	}
	//fprintf(stderr, "%d, ctcp_receive finish!!!\n", getpid());
}

void ctcp_output(ctcp_state_t *state) {
	/* FIXME */
	//fprintf(stderr, "%d, call ctcp_output\n", getpid());
	int flag = 0;
	int index = state->pkt_expected % state->win_size;
	//fprintf(stderr, "1.index:%d\n", index);
	while (state->arrived[index] && state->in_q[index].size && conn_bufspace(state->conn) >= state->in_q[index].size){
		conn_output(state->conn, state->in_q[index].data, state->in_q[index].size);
		//fprintf(stderr, "%d, 1.ctcp_output-->conn_output finished!!!\n", getpid());
		state->arrived[index] = 0;
		memset(&(state->in_q[index]), '\0', sizeof(state->in_q[index]));
		state->pkt_expected++;
		state->largest_accpeptable_pkt++;
		index = state->pkt_expected % state->win_size;
		flag = 1;
	}
	if (flag == 1){
		send_sgm(state, ACK_PKT, 0);
	}
	//fprintf(stderr, "2.index:%d\n", index);
	if (state->arrived[index] && state->in_q[index].size == 0){
		conn_output(state->conn, state->in_q[index].data, state->in_q[index].size);
		//fprintf(stderr, "%d, 2.ctcp_output-->conn_output finished!!!\n", getpid());
		state->arrived[index] = 0;
		memset(&(state->in_q[index]), '\0', sizeof(state->in_q[index]));
		state->pkt_expected++;
		state->largest_accpeptable_pkt++;
		send_sgm(state, ACK_PKT, 0);
		state->EOF_from_peer = 1;
		state->all_acked = is_all_acked(state);
		if (state->EOF_from_peer == 1 && state->read_EOF_from_input == 1 && state->all_acked == 1){
			ctcp_destroy(state);
			//fprintf(stderr, "%d, ctcp_output-->ctcp_destroy finish!!!\n", getpid());
		}
	}
	//fprintf(stderr, "%d, ctcp_output finish!!!\n", getpid());
}

void ctcp_timer() {
	/* FIXME */
	////fprintf(stderr, "%d, call ctcp_timer\n", getpid());
	ctcp_state_t *state;
	int i, index;
	for (state = state_list; state != NULL; state = state->next){
		for (i = state->ack_expected; i < state->next_pkt_to_send; i++){
			index = i % state->win_size;
			state->ack_timer[index] = state->ack_timer[index] + state->timer;
			if (state->ack_timer[index] >= state->timeout){
				if (state->out_q[index].size == 0){
					if (state->timeout_rounds < RETRANS_ATTMP){
						send_sgm(state, DATA_PKT, state->out_q[index].seq);
						state->timeout_rounds++;
					}
					else{
						ctcp_destroy(state);
						//fprintf(stderr, "%d, ctcp_timer-->ctcp_destroy finish!!!\n", getpid());
					}
				}
				else{
					send_sgm(state, DATA_PKT, state->out_q[index].seq);
				}
			}
		}
	}
	////fprintf(stderr, "%d, ctcp_timer finish!!!\n", getpid());
	
}

