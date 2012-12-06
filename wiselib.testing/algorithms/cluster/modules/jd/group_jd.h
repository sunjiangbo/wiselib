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

#ifndef _GROUP_JD_H
#define	_GROUP_JD_H

#include "util/delegates/delegate.hpp"
#include "util/pstl/vector_static.h"
#include "util/pstl/map_static_vector.h"
#include "algorithms/cluster/clustering_types.h"

namespace wiselib {

    /**
     * \ingroup jd_concept
     *
     * GroupJoinDecision
     */
    template<typename OsModel_P, typename Radio_P, typename Semantics_P >

    class GroupJoinDecision {
    public:

        //TYPEDEFS
        typedef OsModel_P OsModel;
        typedef Radio_P Radio;
        typedef typename OsModel::Debug Debug;
        typedef Semantics_P Semantics_t;
        typedef typename Semantics_t::semantic_id_t semantic_id_t;
        typedef typename Semantics_t::value_t value_t;
        typedef typename Semantics_t::group_entry_t group_entry_t;
        typedef typename Semantics_t::group_container_t group_container_t;
        typedef typename Semantics_t::value_container_t value_container_t;

        //        typedef JoinSemanticClusterMsg_t::semantics_t semantics_t;


        // data types
        typedef typename Radio::node_id_t node_id_t;
        typedef typename Radio::block_data_t block_data_t;
        typedef node_id_t cluster_id_t;

        typedef delegate2<void, group_entry_t, node_id_t> join_delegate_t;
        typedef join_delegate_t notifyAboutGroupDelegate_t;

        struct groups_joined_entry {
            node_id_t group_max_id_;
            node_id_t parent_;
            block_data_t data[30];
            uint8_t size_;
        };
        typedef struct groups_joined_entry groups_joined_entry_t;
        typedef wiselib::vector_static<OsModel, groups_joined_entry_t, 6 > groupsVector_t;
        typedef typename groupsVector_t::iterator groupsVectorIterator_t;


        // --------------------------------------------------------------------

        /**
         * Constructor
         */
        GroupJoinDecision() :
        cluster_id_(Radio::NULL_NODE_ID)
        , hops_(200) {
        };

        /**
         * Destructor
         */
        ~GroupJoinDecision() {
        };

        /**
         *
         * @param radiot
         * wiselib radio
         * @param debugt
         * wiselib debug
         * @param semantics
         * wiselib semantic storage
         */
        void init(Radio& radio, Debug& debug, Semantics_t &semantics) {
            radio_ = &radio;
            debug_ = &debug;
            semantics_ = &semantics;
            groupsVector_.clear();
        };

        void reset() {
            groupsVector_.clear();
        }

        /**
         *
         * @return
         * a SemaGroupMsg_t message ready to be sent using the radio
         */
        SemaGroupsMsg_t get_join_payload() {
            //            debug_->debug("payload");
            SemaGroupsMsg_t msg;
            //            semantics_vector_.clear();
            group_container_t mygroups = semantics_->get_groups();

            for (typename group_container_t::iterator gi = mygroups.begin(); gi != mygroups.end(); ++gi) {
                //                                debug_->debug("adding semantic size - %d : to add size %d", msg.length(), sizeof (uint8_t) + gi->size());
                msg.add_statement(gi->data(), gi->size(), group_id(*gi), group_parent(*gi));
            }
            msg.set_node_id(radio_->id());

            //            debug_->debug("created array msgsize is %d contains %d", msg.length(), msg.contained());

            return msg;
        }

        /**
         * 
         * @param gi
         * the group 
         * @return
         * the maximum id of the given group
         */
        node_id_t group_id(group_entry_t gi) {
            if (!groupsVector_.empty()) {
                for (typename groupsVector_t::iterator it = groupsVector_.begin(); it != groupsVector_.end(); ++it) {
                    //if of the same size maybe the same
                    if (gi.size() == it->size_) {
                        bool same = true;
                        //byte to byte comparisson
                        for (uint8_t i = 0; i < it->size_; i++) {
                            if (it->data[i] != *(gi.data() + i)) {
                                same = false;
                                break;
                            }
                        }

                        if (same) {
                            return it->group_max_id_;
                        }
                    }
                }
            }
            groups_joined_entry_t newentry;
            newentry.size_ = gi.size();
            newentry.group_max_id_ = radio().id();
            newentry.parent_ = radio().id();
            memcpy(newentry.data, gi.data(), gi.size());
            groupsVector_.push_back(newentry);
            return radio().id();
        }

        /**
         * 
         * @param gi
         * the group
         * @return
         * the parent id of the given group
         */
        node_id_t group_parent(group_entry_t gi) {
            if (!groupsVector_.empty()) {
                for (typename groupsVector_t::iterator it = groupsVector_.begin(); it != groupsVector_.end(); ++it) {
                    //if of the same size maybe the same
                    if (gi.size() == it->size_) {
                        bool same = true;
                        //byte to byte comparisson
                        for (uint8_t i = 0; i < it->size_; i++) {
                            if (it->data[i] != *(gi.data() + i)) {
                                same = false;
                                break;
                            }
                        }

                        if (same) {
                            return it->parent_;
                        }
                    }
                }
            }
            return Radio::NULL_NODE_ID;
        }

        /**
         *          
         * @return
         * the parent id of the given group
         */
        node_id_t group_parent() {
            if (!groupsVector_.empty()) {
                return groupsVector_.begin()->parent_;                     
            }
            return Radio::NULL_NODE_ID;
        }

        /**
         * 
         * @param gi
         * the group entry
         * @param group_id
         * the groups max known id
         * @param parent
         * the parent of the node in the group
         */
        void set_my_group_id(group_entry_t gi, node_id_t group_id, node_id_t parent) {
            if (!groupsVector_.empty()) {
                for (typename groupsVector_t::iterator it = groupsVector_.begin(); it != groupsVector_.end(); ++it) {
                    //if of the same size maybe the same
                    if (gi.size() == it->size_) {
                        bool same = true;
                        //byte to byte comparisson
                        for (uint8_t i = 0; i < it->size_; i++) {
                            if (it->data[i] != *(gi.data() + i)) {
                                same = false;
                                break;
                            }
                        }

                        if (same) {
                            it->group_max_id_ = group_id;
                            it->parent_ = parent;
                        }
                    }
                }
            }
        }

        /**
         *
         * @param node
         * the dropped node
         * @return
         * true if the event resulted in cluster changes
         */
        bool node_lost(node_id_t node) {
            bool changed = false;
            if (!groupsVector_.empty()) {
                for (typename groupsVector_t::iterator it = groupsVector_.begin(); it != groupsVector_.end(); ++it) {
                    if (it->group_max_id_ == node) {
                        it->parent_ = radio().id();
                        it->group_max_id_ = radio().id();
                        group_entry_t gi = group_entry_t(it->data, it->size_, semantics_->get_allocator());
                        //                        debug().debug("CLL;%x;%s-%x;%x", radio().id(), gi.c_str(), it->group_max_id_, it->parent_);
                        changed = true;
                        joined_group(gi, it->parent_);
                    }
                }
            }
            return changed;
        }

        /**
         * respond to an JOIN message received
         * @param mess
         * pointer to the message payload
         * @param len
         * size of the message payload
         * @return
         * true when joined any cluster else false
         */
        bool join(uint8_t * mess, size_t len) {
            bool joined_any = false;
            SemaGroupsMsg_t * msg = (SemaGroupsMsg_t *) mess;
            uint8_t group_count = msg->contained();
            //            debug_->debug("contains %d ,len : %d", group_count, len);

            for (uint8_t i = 0; i < group_count; i++) {

                group_entry_t gi = group_entry_t(msg->get_statement_data(i), msg->get_statement_size(i));

                //                debug().debug("got a msg for %s nid %x", gi.c_str(), msg->get_statement_nodeid(i));
                if (semantics_-> has_group(gi)) {
                    //                    debug().debug("Received;%s;{id:%x parent:%x};{id:%x parent:%x}", gi.c_str(), msg->get_statement_nodeid(i), msg->get_statement_parent(i), group_id(gi), group_parent(gi));

                    // if he is my parent in the group
                    if (group_parent(gi) == msg->node_id()) {

                        //if he thinks i am his parent resolve  - lose him

                        if (msg->get_statement_parent(i) == radio().id()) {
                            if (radio().id() > msg->node_id()) {
                                set_my_group_id(gi, radio().id(), radio().id());
                            } else {
                                set_my_group_id(gi, msg->node_id(), msg->node_id());
                            }
                            //                            debug().debug("CLL;%x;%s-%x;%x", radio().id(), gi.c_str(), group_id(gi), group_parent(gi));
                            joined_group(gi, group_parent(gi));
                        }

                        if (group_id(gi) != msg->get_statement_nodeid(i)) {
                            set_my_group_id(gi, msg->get_statement_nodeid(i), msg->node_id());
                            joined_group(gi, group_parent(gi));
                        }
                    }

                    if (group_id(gi) < msg->get_statement_nodeid(i)) {
                        joined_any = true;
                        set_my_group_id(gi, msg->get_statement_nodeid(i), msg->node_id());

                        //                        debug().debug("CLL;%x;%s-%x;%x", radio().id(), gi.c_str(), group_id(gi), msg->node_id());
                        joined_group(gi, msg->node_id());
                    }
                }
            }

            return joined_any;
        }

        /**
         * enables the module initializes values
         */
        void enable() {

        };

        /**         
         * disables this module unregisters callbacks
         */
        void disable() {
        };

        template<class T, void (T::*TMethod)(group_entry_t, node_id_t) >
        int reg_group_joined_callback(T * obj_pnt) {
            join_delegate_ = join_delegate_t::template from_method<T, TMethod > (obj_pnt);
            return join_delegate_;
        }
        // --------------------------------------------------------------------

        int unreg_group_joined_callback(int idx) {
            join_delegate_ = join_delegate_t();
            return idx;
        }
        // --------------------------------------------------------------------

        void joined_group(group_entry_t group, node_id_t parent) {

            if (join_delegate_ != join_delegate_t()) {
                (join_delegate_) (group, parent);
            }
        }

        template<class T, void (T::*TMethod)(group_entry_t, node_id_t) >
        int reg_notifyAboutGroup_callback(T * obj_pnt) {
            notifyAboutGroupDelegate_ = notifyAboutGroupDelegate_t::template from_method<T, TMethod > (obj_pnt);
            return notifyAboutGroupDelegate_;
        }
        // --------------------------------------------------------------------

        int unreg_notifyAboutGroup_callback(int idx) {
            notifyAboutGroupDelegate_ = notifyAboutGroupDelegate_t();
            return idx;
        }
        // --------------------------------------------------------------------

        void notifyAboutGroup(group_entry_t group, node_id_t parent) {

            if (notifyAboutGroupDelegate_ != notifyAboutGroupDelegate_t()) {
                (notifyAboutGroupDelegate_) (group, parent);
            }
        }

        /**
         *
         * @param gi
         * the group to check for its parent
         * @return
         * the parent of the group
         */
        inline node_id_t parent(group_entry_t gi) {
            if (!groupsVector_.empty()) {
                for (groupsVectorIterator_t it = groupsVector_.begin(); it != groupsVector_.end(); ++it) {
                    //if of the same size maybe the same
                    if (gi.size() == it->size_) {
                        bool same = true;
                        //byte to byte comparisson
                        for (uint8_t i = 0; i < it->size_; i++) {
                            if (it->data[i] != *(gi.data() + i)) {
                                same = false;
                                break;
                            }
                        }
                        if (same) {
                            return it->parent_;
                        }
                    }
                }
            }
            return Radio::NULL_NODE_ID;
        }

    private:
        join_delegate_t join_delegate_;
        notifyAboutGroupDelegate_t notifyAboutGroupDelegate_;
        cluster_id_t cluster_id_;
        int hops_;
        Semantics_t * semantics_;
        groupsVector_t groupsVector_;

        Radio * radio_; //radio module
        Debug * debug_; //debug module

        Radio & radio() {
            return *radio_;
        }

        Debug & debug() {
            return *debug_;
        }
    };
}
#endif
