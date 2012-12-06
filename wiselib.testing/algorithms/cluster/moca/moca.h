/***************************************************************************
 ** This file is part of the generic algorithm library Wiselib.           **
 ** Copyright (C) 2008,2009 by the Wisebed (www.wisebed.eu) project.      **
 **                                                                       **
 ** The Wiselib is free software: you can redistribute it and/or modify   **
 ** it under the terms of the GNU Lesser General Public License as        **
 ** published by the Free Software Foundation, either version 3 of the    **
 ** License, or (at your option) any later version.                       **
 **                                                                       **
 ** The Wiselib is distributed in the hope that it will be useful,        **
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of        **
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
 ** GNU Lesser General Public License for more details.                   **
 **                                                                       **
 ** You should have received a copy of the GNU Lesser General Public      **
 ** License along with the Wiselib.                                       **
 ** If not, see <http://www.gnu.org/licenses/>.                           **
 ***************************************************************************/

/*
 * File:   moca_core.h
 * Author: amaxilat
 *
 */

#ifndef _MOCA_CORE_H
#define	_MOCA_CORE_H

#include "util/delegates/delegate.hpp"
#include "algorithms/cluster/clustering_types.h"
#include "util/base_classes/clustering_base.h"

#include "algorithms/cluster/modules/chd/prob_chd.h"
#include "algorithms/cluster/modules/jd/moca_jd.h"
#include "algorithms/cluster/modules/it/moca_it.h"

//ECHO PROTOCOL
#include "algorithms/neighbor_discovery/echo.h"
#include "algorithms/neighbor_discovery/pgb_payloads_ids.h"




#define MAINTENANCE

#undef DEBUG
// Uncomment to enable Debug
#define DEBUG
#ifdef DEBUG
//#define DEBUG_EXTRA
//#define DEBUG_RECEIVED
#define DEBUG_CLUSTERING
#endif

namespace wiselib {

    /** \brief Moca clustering core component.
     * 
     *  \ingroup cc_concept
     *  \ingroup basic_algorithm_concept
     *  \ingroup clustering_algorithm
     * 
     */
    template<typename OsModel_P, typename Radio_P, typename HeadDecision_P,
    typename JoinDecision_P, typename Iterator_P>
    class MocaCore : public ClusteringBase <OsModel_P> {
    public:
        //TYPEDEFS
        typedef OsModel_P OsModel;
        // os modules
        typedef Radio_P Radio;
        typedef typename OsModel::Timer Timer;
        typedef typename OsModel::Debug Debug;
        typedef typename OsModel::Rand Rand;
        //algorithm modules
        typedef HeadDecision_P HeadDecision_t;
        typedef JoinDecision_P JoinDecision_t;
        typedef Iterator_P Iterator_t;
        // self_type
        typedef MocaCore<OsModel_P, Radio_P, HeadDecision_P, JoinDecision_P, Iterator_P > self_type;
        typedef wiselib::Echo<OsModel, Radio, Timer, Debug> nb_t;
        // data types
        typedef int cluster_level_t; //quite useless within current scheme, supported for compatibility issues
        typedef typename Radio::node_id_t node_id_t;
        typedef node_id_t cluster_id_t;
        typedef typename Radio::size_t size_t;
        typedef typename Radio::block_data_t block_data_t;


        // delegate
        //typedef delegate1<void, int> cluster_delegate_t;

        /**
         * Constructor
         */
        MocaCore() :
        enabled_(false),
        status_(0),
        probability_(30),
        maxhops_(3),
        auto_reform_(0),
        reform_(false),
        head_lost_(false),
        do_cleanup(false) {
        }

        /**
         * Destructor
         */
        ~MocaCore() {
        }

        /**
         * initializes the values of radio timer and debug
         */
        void init(Radio& radiot, Timer& timert, Debug& debugt, Rand& randt, nb_t& neighbor_discovery) {
            radio_ = &radiot;
            timer_ = &timert;
            debug_ = &debugt;
            rand_ = &randt;

            neighbor_discovery_ = &neighbor_discovery;

            uint8_t flags = nb_t::DROPPED_NB | nb_t::LOST_NB_BIDI | nb_t::NEW_PAYLOAD_BIDI;

            neighbor_discovery_->template reg_event_callback<self_type,
                    &self_type::ND_callback > (CLUSTERING, flags, this);
            neighbor_discovery_->register_payload_space((uint8_t) CLUSTERING);

            jd().template reg_cluster_joined_callback<self_type, &self_type::joined_cluster > (this);

            //cradio_delegate_ = cradio_delegate_t();

            //initialize the clustering modules
            chd().init(radio(), debug(), rand());
            jd().init(radio(), debug());
            it().init(radio(), timer(), debug());
        }

        /*
         * Set Clustering modules
         * it,jd,chd
         */
        inline void set_iterator(Iterator_t &it) {
            it_ = &it;
        }

        inline void set_join_decision(JoinDecision_t &jd) {
            jd_ = &jd;
        }

        inline void set_cluster_head_decision(HeadDecision_t &chd) {
            chd_ = &chd;
        }

        /**
         * Set Clustering Parameters
         * maxhops probability
         */
        void set_probability(int prob) {
            probability_ = prob;
        }

        void set_maxhops(int maxhops) {
            maxhops_ = maxhops;
        }

        /* Get Clustering Values
         * cluster_id
         * parent
         * hops (from parent)
         * node_type
         * childs_count
         * childs
         * node_count (like childs)
         * is_gateway
         * is_cluster_head
         */
        inline cluster_id_t cluster_id(size_t cluster_no = 0) {
            return it().cluster_id(cluster_no);
        }

        inline node_id_t parent(cluster_id_t cluster_id = 0) {
            return it().parent(cluster_id);
        }

        inline size_t hops(cluster_id_t cluster_id = 0) {
            return it().hops(cluster_id);
        }

        inline int node_type() {
            return it().node_type();
        }

        inline size_t clusters_joined() {
            return it().clusters_joined();
        }

        //TODO: CHILDS_COUNT
        //TODO: CHILDS
        //TODO: NODE_COUNT
        //TODO: IS_GATEWAY

        inline bool is_cluster_head() {
            if (!enabled_) return false;
            return it().is_cluster_head();
        }

        /**
         SHOW all the known nodes
         */

        void present_neighbors(void) {
            if (status() != UNFORMED) {
                it().present_neighbors();
            }
        }

        /**
         Self Register a debug callback
         */
        void register_debug_callback() {
            this-> template reg_state_changed_callback<self_type, &self_type::debug_callback > (this);
        }

        void debug_callback(int event) {
            switch (event) {
                case ELECTED_CLUSTER_HEAD:
                case CLUSTER_FORMED:
                case NODE_JOINED:
                    debug().debug("CLP;%x;%d;%x", radio().id(), it().node_type(), it().cluster_id());
                    return;
                case MESSAGE_SENT:
                    debug().debug("CLS;%x;45;%x", radio().id(), 0xffff);
                    return;

            }
        }

        /*
         * The status Of the Clustering Algorithm
         * 1 means a cluster is being formed
         * 0 means cluster is formed
         */
        inline uint8_t status() {
            //1 - forming , 0 - formed
            if (enabled_) {
                return status_;
            } else {
                return UNFORMED;
            }
        }

        /*
         * Receive a beacon payload
         * check for new head if needed
         * check if in need to reform
         */
        void receive_beacon(node_id_t node_from, size_t len, uint8_t * data) {

            if (!enabled_) return;
            if (data[0] == JOINM) {
                receive(node_from, len, data);
            }
        }

        /*
         * Enable
         * enables the clustering module
         * enable chd it and jd modules
         * calls find head to start clustering
         */
        inline void enable() {
#ifdef SHAWN
            //typical time for shawn to form stable links
            enable(6);
#else
            //typical time for isense test to form stable links
            enable(40);
#endif
        }

        inline void enable(int start_in) {
            if (enabled_) return;

            //set as enabled
            enabled_ = true;
            head_lost_ = false;

            // receive receive callback
            callback_id_ = radio().template reg_recv_callback<self_type,
                    &self_type::receive > (this);

#ifdef DEBUG_EXTRA
            debug().debug("CL;%x;enable", radio().id());
#ifdef SHAWN
            debug().debug("\n");
#endif
#endif

            chd().reset();
            jd().reset();
            it().reset();

            // set variables of other modules
            chd().set_probability(probability_);
            jd().set_maxhops(maxhops_);

#ifndef FIXED_ROLES

            timer().template set_timer<self_type, &self_type::form_cluster > (
                    start_in * 1000, this, (void *) maxhops_);

#endif
        }

        /*
         * Disable     
         */
        void disable() {
            if (!enabled_) return;
            // Unregister the callback
            radio().unreg_recv_callback(callback_id_);
            enabled_ = false;
        }

        // --------------------------------------------------------------------

        void reset_beacon_payload() {
            if (!enabled_) return;
            //reset my beacon according to the new status
            if (clusters_joined() > 0) {
                JoinMultipleClusterMsg_t msg = it().get_join_request_payload();
                neighbor_discovery_->set_payload((uint8_t) CLUSTERING, (uint8_t*) & msg, msg.length());
            }
        }


    protected:


        // Call with a timer to start a reform procedure from the cluster head

        inline void reform_cluster(void * parameter) {
            if (!enabled_) return;
            reform_ = true;
        }

        void form_cluster(void * parameter) {
            if (!enabled_) return;
            status_ = FORMING;
            //enabling
            chd().reset();
            chd().set_probability(probability_);
            jd().reset();
            jd().set_maxhops(maxhops_);
            it().reset();

            //            JoinMultipleClusterMsg_t msg = it().get_join_request_payload();
            //
            //            neighbor_discovery_->set_payload((uint8_t) CLUSTERING, (uint8_t *) & msg, msg.length());

            // start the procedure to find new head
            timer().template set_timer<self_type, &self_type::find_head > (
                    rand()() % 300 + time_slice_, this, (void *) 0);

            // reform is false as cluster is not yet formed
            reform_ = false;
        }

        /*
         * FIND_HEAD
         * starts clustering
         * decides a head and then start clustering
         * from the head of each cluster
         * */
        void find_head(void * value) {
#ifdef DEBUG_EXTRA
            debug().debug("CL;stage1;Clusterheaddecision");
#endif
            // if Cluster Head
            if (chd().calculate_head() == true) {

                //Node is head now
                // set values for iterator and join_decision
                it().set_node_type(HEAD);
                this->state_changed(ELECTED_CLUSTER_HEAD);

#ifdef DEBUG_EXTRA
                debug().debug("CL;stage2;Join");
#endif

                //jd(). get join payload
                JoinMultipleClusterMsg_t join_msg = it().get_join_request_payload();

                // send JOIN
                radio().send(Radio::BROADCAST_ADDRESS, join_msg.length(), (uint8_t *) & join_msg);
#ifdef DEBUG_CLUSTERING
                debug().debug("CLS;%x;%d;%x;len%d", radio().id(), join_msg.msg_id(), Radio::BROADCAST_ADDRESS, join_msg.length());
#ifdef SHAWN
                debug().debug("\n");
#endif
#endif
                //this->state_changed(MESSAGE_SENT);

                //                //set the time to check for finished clustering
                //                timer().template set_timer<self_type,
                //                        &self_type::wait2form_cluster > (2 * maxhops_ * time_slice_, this, (void*) 0);

            }
            timer().template set_timer<self_type,
                    &self_type::reply_to_head > (maxhops_ * time_slice_, this, (void*) 0);
        }

        /*
         * wait2form_cluster
         * if wait2form_cluster is called
         * clustering procedure ends
         * */
        //        void wait2form_cluster(void *) {
        //            if (!enabled_)return;
        //            if (is_cluster_head()) {
        //                // if a cluster head end the clustering under this branch
        //#ifdef DEBUG_CLUSTERING
        //                //                debug().debug("CLP;%x;%d;%x", radio().id(), it().node_type(), it().cluster_id());
        //#ifdef SHAWN
        //                debug().debug("\n");
        //#endif
        //#endif
        //                this->state_changed(CLUSTER_FORMED);
        //                reset_beacon_payload();
        //            }
        //        }

        //TODO:REPORT TO HEADS

        void reply_to_head(void *) {
#ifdef DEBUG_EXTRA
            debug().debug("CL;stage3;Reply");
#endif

            if (it().clusters_joined() < 1) {
#ifdef DEBUG_CLUSTERING
                //                debug().debug("CL;Node Joined no cluster, change to cluster_head");
                debug().debug("CL;Uncovered");
#endif
                it().set_node_type(HEAD);
                // Cluster is head now
                this->state_changed(ELECTED_CLUSTER_HEAD);
                //
                //                timer().template set_timer<self_type,
                //                        &self_type::wait2form_cluster > (maxhops_ * time_slice_, this, (void*) 0);

            } else {
#ifdef DEBUG_EXTRA
                debug().debug("CL;Node%x,reply %d clusters", radio().id(), it().clusters_joined());
#endif

                send_convergecast(0);

            }
            reset_beacon_payload();
        }

        void send_convergecast(void * counter) {
            int count = (int) counter;
            //debug().debug("sending to cluster %d", count);



            ConvergecastMsg_t mess = it().get_resume_payload();
            debug().debug("checking for %d,%d", count, count < it().clusters_joined());
            if (count < it().clusters_joined()) {
                cluster_id_t destination_cluster = it().cluster_id(count);
                if ((destination_cluster != radio().id()) && (destination_cluster != 0x0) && (destination_cluster != 0xffff)) {
                    mess.set_cluster_id(destination_cluster);
                    radio().send(it().parent(destination_cluster), mess.length(), (uint8_t *) & mess);
#ifdef DEBUG_CLUSTERING
                    debug().debug("CLS;%x;%d;%x; for %x|%d|%d", radio().id(), mess.msg_id(), it().parent(destination_cluster), destination_cluster, count, it().clusters_joined());
#endif
                    //this->state_changed(MESSAGE_SENT);

                }
                //debug().debug("sent to cluster %d", count);
                count++;
                if (count < it().clusters_joined()) {
                    timer().template set_timer <self_type, &self_type::send_convergecast > (400, this, (void*) count);
                }
            } else {
                //timer().template set_timer <self_type, &self_type::send_convergecast > (2000, this, (void*) 0);
            }
        }

        void joined_cluster(cluster_id_t cluster, int hops, node_id_t parent) {
            if (it().add_cluster(cluster, hops, parent)) {
                //reset_beacon_payload();
                this->state_changed(NODE_JOINED);
#ifdef DEBUG_CLUSTERING
                debug().debug("CLP;%x;%d;%x", radio().id(), radio().id() == cluster ? HEAD : SIMPLE, cluster);
#endif
            }
        }

        /*
         * Called when ND lost contact with a node
         * If the node was cluster head
         *  - start searching for new head
         * else
         *  - remove node from known nodes
         */
        void node_lost(node_id_t node) {
            if (!enabled_) return;
            it().drop_node(node);
            //            if (status() == FORMED) {
            //If the node was my route to CH

            //                    if (it().clusters_joined() == 1) {
            //                        //Reset Iterator
            //                        it().reset();
            //                        jd().reset();
            //                        //Mark as headless
            //                        head_lost_ = true;
            //                    } else {
            //                        it().drop_node(node);
            //                    }
            //                    //Timeout for new CH beacons
            //                    timer().template set_timer <self_type, &self_type::reply_to_head > (time_slice_, this, (void*) 0);
            //                } else {
            //                    //if not my CH
            //                    //Remove from Iterator
            //                    it().drop_node(node);

            //            }
        }

        /*
         * RECEIVE
         * respond to the new messages received
         * callback from the radio
         * */
        void receive(node_id_t from, size_t len, block_data_t *data) {
            if (!enabled_) return;
            if (from == radio().id()) return;

            if (!neighbor_discovery_->is_neighbor_bidi(from)) return;

            // get Type of Message
            int type = data[0];

            if (type == JOINM) {
                if (jd().join(data, len)) {
                    JoinMultipleClusterMsg_t join_msg;
                    join_msg = it().get_join_request_payload();
                    radio().send(Radio::BROADCAST_ADDRESS, join_msg.length(), (uint8_t*) & join_msg);
#ifdef DEBUG_CLUSTERING
                    debug().debug("CLS;%x;%d;%x", radio().id(), join_msg.msg_id(), Radio::BROADCAST_ADDRESS);
#endif
                    //this->state_changed(MESSAGE_SENT);

                }
            } else if (type == CONVERGECAST) {
                ConvergecastMsg_t * mess = (ConvergecastMsg_t*) data;
#ifdef DEBUG_RECEIVED
                debug().debug("CL;Received;CONVERGECAST;%x;%x", from, mess.sender_id());
#endif
                if (from == it().parent(mess->cluster_id())) return;

                if ((mess->cluster_id() != radio().id()) && (it().parent(mess->cluster_id()) != 0)) {
#ifdef DEBUG
                    debug().debug("CLS;%x;%d;%x for %x", radio().id(), mess->msg_id(), it().parent(mess->cluster_id()), mess->cluster_id()); //mess.sender_id()
#endif
                    radio().send(it().parent(mess->cluster_id()), mess->length(), (uint8_t*) mess);
                    //this->state_changed(MESSAGE_SENT);
                } else {
                    it().eat_request(mess);
                }
            } else {
#ifdef DEBUG_RECEIVED
                debug().debug("CL;Received;%x;%d;%x;UNKNOWN", radio().id(), data[0], from);
#endif
            }
        }

        void notify_cradio(uint8_t event, cluster_id_t from, node_id_t to) {
            if (!enabled_) return;
            //            if (cradio_delegate_ != 0) {
            //                cradio_delegate_(event, from, to);
            //            }
        }



        // --------------------------------------------------------------------

        void ND_callback(uint8_t event, node_id_t from, uint8_t len, uint8_t * data) {
            if (!enabled_) return;
            if (nb_t::NEW_PAYLOAD_BIDI == event) {

                receive_beacon(from, len, data);
                //reset my beacon according to the new status
                reset_beacon_payload();
            } else if ((nb_t::LOST_NB_BIDI == event) || (nb_t::DROPPED_NB == event)) {
#ifndef FIXED_ROLES
#ifdef MAINTENANCE
                node_lost(from);
#endif
#endif
            }
        }

    private:
        nb_t * neighbor_discovery_;
        bool enabled_;
        uint8_t status_; // the status of the clustering algorithm
        int callback_id_; // receive message callback
        int probability_; // clustering parameter
        int maxhops_; // clustering parameter
        static const uint32_t time_slice_ = 2000; // time to wait for cluster accept replies
        int auto_reform_; //time to autoreform the clusters
        bool reform_; // flag to start reforming
        bool head_lost_; // flag when the head was lost
        bool do_cleanup;


        /* CLustering algorithm modules */
        HeadDecision_t * chd_;

        HeadDecision_t& chd() {
            return *chd_;
        }
        JoinDecision_t * jd_;

        JoinDecision_t& jd() {
            return *jd_;
        }
        Iterator_t * it_;

        Iterator_t& it() {
            return *it_;
        }

        Radio * radio_; // radio module
        Timer * timer_; // timer module
        Debug * debug_; // debug module
        Rand * rand_;

        Radio& radio() {
            return *radio_;
        }

        Timer& timer() {
            return *timer_;
        }

        Debug& debug() {
            return *debug_;
        }

        Rand& rand() {
            return *rand_;
        }
    };
}
#endif	/* _MOCA_MocaCore_H */

