/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef THOMAS_CONFIG_H_
#define THOMAS_CONFIG_H_

#define LOCALHOST "127.0.0.1"
/** When a new neighbour is added via the AddNeighbour function and we already have a known Qvalue for destinations
  * We will take the best estim we have for that dst, add this value in MS to that, then thats the QValuef or the new neighbour
  * that can then be expanded upon by exploration / exploitation as normal
  */
#define NEW_NEIGHBOUR_INITIAL_INCREMENT 3

/** If no more neighbours are available, we return 255.255.255.255 and 9999 ms from the route function
  * this sets that 9999 and the 255.255.255.255 values.
  */
#define NO_NEIGHBOURS_REACHABLE_ROUTE_MS 9999
#define NO_NEIGHBOURS_REACHABLE_ROUTE_IP "212.121.212.121"

/** size of packets (test only) that are incorrectly labelled AODV due to AODV parsing not recognizing SeqTsHeader
  */
#define TEST_UDP_ECHO_PKT_SIZE 987
#define TEST_UDP_NO_ECHO_PKT_SIZE 676

#define UNINITIALIZED_IP_ADDRESS_VALUE "102.102.102.102"
#define UNINITIALIZED_IP_ADDRESS_QINFO_TAG_VALUE "103.103.103.103"
#define UNINITIALIZED_IP_ADDRESS_QLRN_HEADER "104.104.104.104"
#define UNINITIALIZED_IP_ADDRESS_VALUE_QTABLE "233.233.233.233"
#define UNINITIALIZED_IP_ADDRESS_VALUE_PTST "111.222.222.111"
#define INITIAL_QVALUE_QTABLEENTRY_VIA 0 // value in ms is given as initial
#define INITIAL_QVALUE_QTABLEENTRY_NOT_VIA 1 // value in ms is given as initial
#define IP_WHEN_NO_NEXT_HOP_NEIGHBOUR_KNOWN_YET "249.249.249.249"

#define DEFAULT_QLEARNER_INTERVAL 1.0 //(In Seconds)between QLrnPackets (currenctly unused TODO)
#define DEFAULT_EPSILON_VALUE 0.05
#define DEFAULT_GAMMA_VALUE 0.0
#define DEFAULT_LRN_RATE_VALUE 0.5

#define LEARNING_PHASE_START_THRESHOLD 0.60 //in %/100 between estimate / real

#define BOOL_PARAM_SET_TRUE 4444
#define BOOL_PARAM_SET_FALSE 3333

#define VALUE_UNUSED_INT_PARAMETER 123456789
#define VALUE_UNUSED_INT_PARAMETER_STR "123456789"

#define QTABLE_CONVERGENCE_THRESHOLD 0.025
#define Q_INTERMEDIATE_OPTIMAL_RHO 0.99
#define MAX_RETRY_DEFAULT 4
#define FREEZE_ONOFFAPPLICATION_SENDING 998899

#define Q_ROUTE_WILDCARD_VALUE 777777

#define PORT_NUMBER_TRAFFIC_A 10002
#define PORT_NUMBER_TRAFFIC_B 10003
#define PORT_NUMBER_TRAFFIC_C 10004

#define MAX_NR_STRIKES_BLACKLISTED_NODE 5

#endif /* THOMAS_CONFIG_H_ */
