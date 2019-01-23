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

#define UNIT_CONVERT 8
/*add by xu*/
int bitcount(unsigned int n)
{
    unsigned int c =0 ;
    for (c =0; n; ++c)
    {
        n &= (n -1) ; // clear right most 1
    }
    return c ;
}
/*<!-- add by xu for multi-thread */
void qos_send_message_0(struct ccnd_handle *h)
{
  int i = 0;
  int count;
  int size;
  int intvl_b;
  int c_t = 0;
  const char *intvl_c;
  struct timeval tv;
  int bw = 10;
  intvl_c = getenv("CCND_INTVL_G");
  if(intvl_c !=NULL){
     intvl_b = strtoul(intvl_c, NULL, 10);
  }
  else{
     intvl_b = 0;
  }
  long tv_start;
  long tv_end;
  long time_flag;
  long time_flag_c;
  time_flag = 0;
  
  while(i<1000000000000)
 {	
      gettimeofday(&tv, NULL);
	  time_flag_c = tv.tv_sec; //get current time
	  if(time_flag_c - time_flag > 1){
	      h->queue_flag = h->queue_flag & (~QUEUE_0_RUNNING);
	  }
      if(!queue_empty(h->qos_queue_0)){
          h->queue_flag = h->queue_flag | QUEUE_0_RUNNING;
          size = queue_size(h->qos_queue_0);
          for(count = 0; count < size; count ++){
            gettimeofday(&tv, NULL);
            tv_start = tv.tv_usec;
            ccnd_send(h, queue_front(h->qos_queue_0).face, queue_front(h->qos_queue_0).data, queue_front(h->qos_queue_0).size);
	    intvl_b = (int)((float)queue_front(h->qos_queue_0).size*UNIT_CONVERT/(float)bw);
	    //ccnd_msg(h, "Need sleep time: %d", intvl_b);
            //c_t++;
            queue_pop(h->qos_queue_0);
            //ccnd_msg(h, "SIZE_0: %d", queue_front(h->qos_queue_0).size);
            //ccnd_msg(h, "Counter: %d", c_t); 
            gettimeofday(&tv, NULL);
            tv_end = tv.tv_usec;
            if( (intvl_b - tv_end+tv_start) >  0){
                usleep(intvl_b-tv_end+tv_start);
            }
         }
		 gettimeofday(&tv, NULL); // get last time sent packet
		 time_flag = tv.tv_sec;
      }
      i++;
  }

}
/*<!-- add by xu for multi-thread */
void qos_send_message_1(struct ccnd_handle *h)
{
  int i = 0;
  int count;
  int size;
  int intvl_b;
  int c_t = 0;
  const char *intvl_c;
  struct timeval tv;
  int bw = 10;
  intvl_c = getenv("CCND_INTVL_N");
  if(intvl_c !=NULL){
     intvl_b = strtoul(intvl_c, NULL, 10);
  }
  else{
     intvl_b = 0;
  }
  long tv_start;
  long tv_end;
  long time_flag;
  long time_flag_c;
  time_flag = 0;
  
  while(i<1000000000000)
 {
	  gettimeofday(&tv, NULL);
	  time_flag_c = tv.tv_sec;
	  if(time_flag_c - time_flag > 1){
	      h->queue_flag = h->queue_flag & (~QUEUE_1_RUNNING);
	  }
      if(!queue_empty(h->qos_queue_1)){
		  h->queue_flag = h->queue_flag | QUEUE_1_RUNNING;
          size = queue_size(h->qos_queue_1);
          for(count = 0; count < size; count ++){
            gettimeofday(&tv, NULL);
            tv_start = tv.tv_usec;
            ccnd_send(h, queue_front(h->qos_queue_1).face, queue_front(h->qos_queue_1).data, queue_front(h->qos_queue_1).size);
			intvl_b = (int)((float)queue_front(h->qos_queue_1).size*UNIT_CONVERT/(float)bw);
			//ccnd_msg(h, "Need sleep time: %d", intvl_b);
            //c_t++;
            queue_pop(h->qos_queue_1);
            //ccnd_msg(h, "SIZE_1: %d", queue_front(h->qos_queue_1).size);
            //ccnd_msg(h, "Counter: %d", c_t); 
            gettimeofday(&tv, NULL);
            tv_end = tv.tv_usec;
            if( (intvl_b - tv_end+tv_start) >  0){
                usleep(intvl_b-tv_end+tv_start);
            }
         }		 
		 gettimeofday(&tv, NULL);
		 time_flag = tv.tv_sec;
      }
      i++;
  }
}

/*<!-- add by xu for multi-thread */
void qos_send_message_2(struct ccnd_handle *h)
{
  int i = 0;
  int count;
  int size;
  int intvl_b;
  int c_t = 0;
  int bw = 10;
  const char *intvl_c;
  struct timeval tv;
  intvl_c = getenv("CCND_INTVL_R");
  if(intvl_c !=NULL){
     intvl_b = strtoul(intvl_c, NULL, 10);
  }
  else{
     intvl_b = 0;
  }
  long tv_start;
  long tv_end;
  long time_flag;
  long time_flag_c;
  time_flag = 0;
  while(i<1000000000000)
 {	  
      gettimeofday(&tv, NULL);
	  time_flag_c = tv.tv_sec;
	  if(time_flag_c - time_flag > 1){
	      h->queue_flag = h->queue_flag & (~QUEUE_2_RUNNING);
	  }
      if(!queue_empty(h->qos_queue_2)){
		  h->queue_flag = h->queue_flag | QUEUE_2_RUNNING;
          size = queue_size(h->qos_queue_2);
          for(count = 0; count < size; count ++){
            gettimeofday(&tv, NULL);
            tv_start = tv.tv_usec;
            ccnd_send(h, queue_front(h->qos_queue_2).face, queue_front(h->qos_queue_2).data, queue_front(h->qos_queue_2).size);
			intvl_b = (int)((float)queue_front(h->qos_queue_2).size*UNIT_CONVERT/(float)bw);
			//ccnd_msg(h, "Need sleep time: %d", intvl_b);
            //c_t++;
            queue_pop(h->qos_queue_2);
            //ccnd_msg(h, "SIZE_2: %d", queue_front(h->qos_queue_2).size);
            //ccnd_msg(h, "Counter: %d", c_t); 
            gettimeofday(&tv, NULL);
            tv_end = tv.tv_usec;
            if( (intvl_b - tv_end+tv_start) >  0){
                 usleep(intvl_b-tv_end+tv_start);
            }
         }
		 gettimeofday(&tv, NULL);
		 time_flag = tv.tv_sec;
      }
      i++;
  }

}


void norm_send_message(struct ccnd_handle *h)
{
  int i = 0;
  int intvl_b;
  int count;
  int size;
  const char *intvl_c;
  struct timeval tv;
  
  intvl_c = getenv("CCND_INTVL_D");
  if(intvl_c !=NULL){
     intvl_b = strtoul(intvl_c, NULL, 10);
  }
  else{
     intvl_b = 0;
  }
  ccnd_msg(h,"intvl_b: %d", intvl_b);
  
  long tv_start;
  long tv_end;
  int bw = 20;
  while(i<1000000000000)
 {
      if(!queue_empty(h->norm_queue)){
		  switch(bitcount(h->queue_flag)){
			  case 0:
			      //ccnd_msg(h, "Zero guarantee queue running: 0");
                              bw = 20; 
			      break;
			  case 1:
			      //ccnd_msg(h, "ONE guarantee queue running: 1");
                              bw = 10;
			      break;
			  //case 2:
			      //ccnd_msg(h, "Two guarantee queue running: 2");
                             // bw = 10;
		             // break;
			  //case 3:
			      //ccnd_msg(h, "Three guarantee queue running: 3");
                              //bw = 5;
			      //break;
			  default:
			      ccnd_msg(h, "Default: No guarantee queue running: Error");// could not happen
			      break;
		  }
          size = queue_size(h->norm_queue);
          for(count = 0; count < size; count ++){
             gettimeofday(&tv, NULL);
             tv_start = tv.tv_usec;
             ccnd_send(h, queue_front(h->norm_queue).face, queue_front(h->norm_queue).data, queue_front(h->norm_queue).size);
             intvl_b = (int)((float)queue_front(h->norm_queue).size*UNIT_CONVERT/(float)bw);
			 //ccnd_msg(h, "Need sleep time: %d", intvl_b);
             queue_pop(h->norm_queue);
             gettimeofday(&tv, NULL);
             tv_end = tv.tv_usec;
             if( (intvl_b - tv_end+tv_start) >  0){
                 usleep(intvl_b-tv_end+tv_start);
             }
         }
      }
      i++;
  }
}
/* add by xu end -->*/

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
	
    /*<!-- add by xu for multi-thread */
    int ret_thrd0;
    int ret_thrd1;
    int ret_thrd2;
    int ret_thrd3;
    pthread_t thread0;
    pthread_t thread1;
    pthread_t thread2;
    pthread_t thread3;
    /* add by xu end -->*/

    if (argc > 1) {
        fprintf(stderr, "%s", ccnd_usage_message);
        exit(1);
    }
    signal(SIGPIPE, SIG_IGN);
    h = ccnd_create(argv[0], stdiologger, stderr);
    
    /*<!-- add by xu multi-thread */
    ret_thrd0 = pthread_create(&thread0, NULL, (void *)&qos_send_message_0, h);
    if(ret_thrd0!=0){ccnd_msg(h, "Thread-0 create failed!");exit(0);}
    ret_thrd1 = pthread_create(&thread1, NULL, (void *)&qos_send_message_1, h);
    if(ret_thrd1!=0){ccnd_msg(h, "Thread-1 create failed!");exit(0);}
    ret_thrd2 = pthread_create(&thread2, NULL, (void *)&qos_send_message_2, h);
    if(ret_thrd2!=0){ccnd_msg(h, "Thread-2 create failed!");exit(0);}
    ret_thrd3 = pthread_create(&thread3, NULL, (void *)&norm_send_message,  h);
    if(ret_thrd3!=0){ccnd_msg(h, "Thread-3 create failed!");exit(0);}
    /* add by xu end -->*/
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
