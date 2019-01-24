/**
 * @file ccnd_main.c
 *
 * A CCNx program.
 *
 * Copyright (C) 2009-2011, 2013 Palo Alto Research Center, Inc.
 *
 * This work is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 * This work is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details. You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <signal.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <sys/time.h> // add by xu
#include <string.h> //add by xu
#include "ccnd_private.h"
#include <ccn/charbuf.h> //add by xu
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define UNIT_CONVERT 8

/*add by Fumiya for adaptive bandwidth control*/

static double time_diff(struct timeval s, struct timeval e){
    double sec = (double)(e.tv_sec - s.tv_sec);
    double micro = (double)(e.tv_usec - s.tv_usec);
    double passed = sec + micro / 1000.0 / 1000.0;

    return passed;
}

void get_face_address(struct ccnd_handle *h, struct face *face){
    const struct sockaddr *addr = face->addr;
    int port = 0;
    const unsigned char *rawaddr = NULL;
    char printable[80];
    const char *peer = NULL;
    if (addr->sa_family == AF_INET6) {
        const struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        rawaddr = (const unsigned char *)&addr6->sin6_addr;
        port = htons(addr6->sin6_port);
    }
    else if (addr->sa_family == AF_INET) {
        const struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        rawaddr = (const unsigned char *)&addr4->sin_addr.s_addr;
        port = htons(addr4->sin_port);
    }
    if (rawaddr != NULL)
        peer = inet_ntop(addr->sa_family, rawaddr, printable, sizeof(printable));
    if (peer == NULL)
        peer = "(unknown)";
    ccnd_msg(h,
             "id=%d [%s] port %d",
             face->faceid, peer, port);
}


static void
bandwidth_calculation(struct ccnd_handle *h){
	struct timeval tv;
	struct timeval time_last_1sec={0}; //一秒集計用のtimeval
	int k = 0;

    unsigned i;
    int j;
    int n;
    struct face *f;
    struct content_queue *q;
    
    double bw_of_g;
    int bw_of_face;

    while(k<100000000000000){
	gettimeofday(&tv,NULL);
	//queueの更新作業
        for (i = 0; i < h->face_limit ; i++){
            if (h->faces_by_faceid[i] == NULL)
                continue;
            f = h->faces_by_faceid[i];

            for (j = 0;  j<QOS_QUEUE ; j++) {
                if (f->qos_q[j] == NULL)
                    break;
                else{
                    q = f->qos_q[j];
                    if (q->use_flag == 2 && time_diff(q->last_use,tv) >= 0.3 && q->sender == NULL) {
                        if (q->queue_type == CQ_DEFAULT)
                            f->number_of_default_queue--;
                        else
                            f->number_of_guarantee_queue--;
			q->use_flag = 0;
			ccnd_msg(h,"queue delete remianing queue D:%d  G:%d",f->number_of_default_queue, f->number_of_guarantee_queue);
                    }
		    if (q->use_flag == 1 && q->sender == NULL && time_diff(q->last_use,tv) >= 0.3) {
                        q->use_flag = 2;
                        struct timeval last;
                        gettimeofday(&last,NULL);
                        q->last_use = last;
                    }
                }
            }
        }
	//最終更新時間から1秒が経過していた時
	if((tv.tv_sec - time_last_1sec.tv_sec) >= 1){
	    time_last_1sec = tv;

	    for (i = 0; i < h->face_limit ; i++){
	        if (h->faces_by_faceid[i] == NULL)
	            continue;
	        f = h->faces_by_faceid[i];
		    bw_of_g = 3000000.0 * (double)(f->amount_size_of_guarantee - f->send_size_of_guarantee) / (4000.0 * 94.0);
		    if(bw_of_g > 3000000.0){
		        bw_of_g = 3000000.0;
		    }
		    for (j = 0;  j<QOS_QUEUE ; j++) {
		        if (f->qos_q[j] == NULL)
		            break;
		        else{
		            q = f->qos_q[j];
			        if (q->use_flag == 0)
			            continue;
			        if ((f->number_of_default_queue + f->number_of_guarantee_queue) > 0){
			            if (q->queue_type == CQ_DEFAULT) {
			                q->bandwidth = (q->bandwidth_of_face - (int)bw_of_g) / (f->number_of_default_queue + f->number_of_guarantee_queue);
			            }else{
			                q->bandwidth = (q->bandwidth_of_face - (int)bw_of_g) / (f->number_of_default_queue + f->number_of_guarantee_queue) + (int)bw_of_g / f->number_of_guarantee_queue;
			            }
			        }
			        else
			            q->bandwidth = q->bandwidth_of_face;
			        q->remaining_bandwidth = q->bandwidth;
			        q->bw_flag = 0;
                    }
		    }
		    //get_face_address(h,f);
	    }
	}
    }
}
/*add by Fumiya for adaptive bandwidth control*/

static int
stdiologger(void *loggerdata, const char *format, va_list ap)
{
    FILE *fp = (FILE *)loggerdata;
    return(vfprintf(fp, format, ap));
}

int
main(int argc, char **argv)
{
    struct ccnd_handle *h;
    
    /*add by Fumiya for adaptive bandwidth control*/
    int bw_thread;
    pthread_t thread8;
    /*add by Fumiya for adaptive bandwidth control*/

    if (argc > 1) {
        fprintf(stderr, "%s", ccnd_usage_message);
        exit(1);
    }
    signal(SIGPIPE, SIG_IGN);
    h = ccnd_create(argv[0], stdiologger, stderr);
    
    /*add by Fumiya for adaptive bandwidth control*/
    bw_thread = pthread_create(&thread8, NULL, (void *)&bandwidth_calculation, h);
    /*add by Fumiya for adaptive bandwidth control*/

    if (h == NULL)
        exit(1);
    ccnd_run(h);
    ccnd_msg(h, "exiting.");
    ccnd_destroy(&h);
    ERR_remove_state(0);
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    exit(0);
}
