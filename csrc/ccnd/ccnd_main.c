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
    int bw_amount;

    while(k<100000000000000){
	gettimeofday(&tv,NULL);
//	//gListの更新作業
//        for (i = 0; i < h->face_limit ; i++){
//            if (h->faces_by_faceid[i] == NULL)
//                continue;
//            f = h->faces_by_faceid[i];
//
//            for (j = 0;  j<100 ; j++) {
//                if (f->gList[j] == NULL)
//                    break;
//                else{
//                    q = f->gList[j];
//                    if (q->use_flag == 2 && time_diff(q->last_use,tv) >= 0.3 && q->sender == NULL) {
//                        if (q->queue_type == CQ_DEFAULT)
//                            f->number_of_default_queue--;
//                        else
//                            f->number_of_guarantee_queue--;
//			q->use_flag = 0;
//                    }
//		    if (q->use_flag == 1 && q->sender == NULL && time_diff(q->last_use,tv) >= 0.3) {
//                        q->use_flag = 2;
//                        struct timeval last;
//                        gettimeofday(&last,NULL);
//                        q->last_use = last;
//                    }
//                }
//            }
//        }
	//最終更新時間から1秒が経過していた時
	if((tv.tv_sec - time_last_1sec.tv_sec) >= 1){
	    time_last_1sec = tv;

	    for (i = 0; i < h->face_limit ; i++){
	        if (h->faces_by_faceid[i] == NULL)
	            continue;
	        f = h->faces_by_faceid[i];
	        //bandwidth_g : gListによって更新
	        f->bandwidth_g = 3000000;
	        //bandwidth_f : 固定値
	        //send_g_amount : 0
	        f->send_g_amount = 0;
	        //send_d_amount : 0
	        f->send_d_amount = 0;
	        //sending_status : 0;
	        f->sending_status = 0;
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
