/*
 * libretroshare/src/services p3gxschannels.cc
 *
 * GxsChannels interface for RetroShare.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include "services/p3gxschannels.h"
#include "serialiser/rsgxschannelitems.h"

#include <retroshare/rsidentity.h>


#include "retroshare/rsgxsflags.h"
#include <stdio.h>

// For Dummy Msgs.
#include "util/rsrandom.h"
#include "util/rsstring.h"

/****
 * #define GXSCHANNEL_DEBUG 1
 ****/
#define GXSCHANNEL_DEBUG 1

RsGxsChannels *rsGxsChannels = NULL;


#define	 GXSCHANNELS_SUBSCRIBED_META		1
#define  GXSCHANNELS_UNPROCESSED_SPECIFIC	2
#define  GXSCHANNELS_UNPROCESSED_GENERIC	3

#define CHANNEL_PROCESS	 		0x0001
#define CHANNEL_TESTEVENT_DUMMYDATA	0x0002
#define DUMMYDATA_PERIOD		60	// long enough for some RsIdentities to be generated.

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3GxsChannels::p3GxsChannels(RsGeneralDataService *gds, RsNetworkExchangeService *nes, RsGixs* gixs)
    : RsGenExchange(gds, nes, new RsGxsChannelSerialiser(), RS_SERVICE_GXSV1_TYPE_CHANNELS, gixs, channelsAuthenPolicy()), RsGxsChannels(this), GxsTokenQueue(this)
{
	// For Dummy Msgs.
	mGenActive = false;
	mCommentService = new p3GxsCommentService(this,  RS_SERVICE_GXSV1_TYPE_CHANNELS);

	RsTickEvent::schedule_in(CHANNEL_PROCESS, 0);

	// Test Data disabled in repo.
	//RsTickEvent::schedule_in(CHANNEL_TESTEVENT_DUMMYDATA, DUMMYDATA_PERIOD);

}

uint32_t p3GxsChannels::channelsAuthenPolicy()
{
	uint32_t policy = 0;
	uint32_t flag = 0;

	flag = GXS_SERV::MSG_AUTHEN_ROOT_PUBLISH_SIGN | GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::RESTRICTED_GRP_BITS);

	flag |= GXS_SERV::MSG_AUTHEN_CHILD_PUBLISH_SIGN;
	//flag |= GXS_SERV::MSG_AUTHEN_ROOT_AUTHOR_SIGN;
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PUBLIC_GRP_BITS);
	RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PRIVATE_GRP_BITS);

	return policy;
}



void p3GxsChannels::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
	std::cerr << "p3GxsChannels::notifyChanges()";
	std::cerr << std::endl;

	/* iterate through and grab any new messages */
	std::list<RsGxsGroupId> unprocessedGroups;

	std::vector<RsGxsNotify *>::iterator it;
	for(it = changes.begin(); it != changes.end(); it++)
	{
		RsGxsMsgChange *msgChange = dynamic_cast<RsGxsMsgChange *>(*it);
		if (msgChange)
		{
			std::cerr << "p3GxsChannels::notifyChanges() Found Message Change Notification";
			std::cerr << std::endl;

			std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgChangeMap = msgChange->msgChangeMap;
			std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::iterator mit;
			for(mit = msgChangeMap.begin(); mit != msgChangeMap.end(); mit++)
			{
				std::cerr << "p3GxsChannels::notifyChanges() Msgs for Group: " << mit->first;
				std::cerr << std::endl;

				if (autoDownloadEnabled(mit->first))
				{
					std::cerr << "p3GxsChannels::notifyChanges() AutoDownload for Group: " << mit->first;
					std::cerr << std::endl;

					/* problem is most of these will be comments and votes, 
					 * should make it occasional - every 5mins / 10minutes TODO */
					unprocessedGroups.push_back(mit->first);
				}
			}
		}
	}

	request_SpecificSubscribedGroups(unprocessedGroups);


	 RsGxsIfaceHelper::receiveChanges(changes);
}

void	p3GxsChannels::service_tick()
{
	dummy_tick();
	RsTickEvent::tick_events();
	return;
}

bool p3GxsChannels::getGroupData(const uint32_t &token, std::vector<RsGxsChannelGroup> &groups)
{
	std::cerr << "p3GxsChannels::getGroupData()";
	std::cerr << std::endl;

	std::vector<RsGxsGrpItem*> grpData;
	bool ok = RsGenExchange::getGroupData(token, grpData);
		
	if(ok)
	{
		std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();
		
		for(; vit != grpData.end(); vit++)
		{
			RsGxsChannelGroupItem* item = dynamic_cast<RsGxsChannelGroupItem*>(*vit);
			if (item)
			{
				RsGxsChannelGroup grp;
				item->toChannelGroup(grp, true);
				delete item;
				groups.push_back(grp);
			}
			else
			{
				std::cerr << "p3GxsChannels::getGroupData() ERROR in decode";
				std::cerr << std::endl;
			}
		}
	}
	else
	{
		std::cerr << "p3GxsChannels::getGroupData() ERROR in request";
		std::cerr << std::endl;
	}

	return ok;
}

/* Okay - chris is not going to be happy with this...
 * but I can't be bothered with crazy data structures
 * at the moment - fix it up later
 */

bool p3GxsChannels::getPostData(const uint32_t &token, std::vector<RsGxsChannelPost> &msgs)
{
	std::cerr << "p3GxsChannels::getPostData()";
	std::cerr << std::endl;

	GxsMsgDataMap msgData;
	bool ok = RsGenExchange::getMsgData(token, msgData);
		
	if(ok)
	{
		GxsMsgDataMap::iterator mit = msgData.begin();
		
		for(; mit != msgData.end();  mit++)
		{
			RsGxsGroupId grpId = mit->first;
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();
		
			for(; vit != msgItems.end(); vit++)
			{
				RsGxsChannelPostItem* item = dynamic_cast<RsGxsChannelPostItem*>(*vit);
		
				if(item)
				{
					RsGxsChannelPost msg;
					item->toChannelPost(msg, true);
					msgs.push_back(msg);
					delete item;
				}
				else
				{
					std::cerr << "Not a GxsChannelPostItem, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}
	else
	{
		std::cerr << "p3GxsChannels::getPostData() ERROR in request";
		std::cerr << std::endl;
	}
		
	return ok;
}


bool p3GxsChannels::getRelatedPosts(const uint32_t &token, std::vector<RsGxsChannelPost> &msgs)
{
	std::cerr << "p3GxsChannels::getRelatedPosts()";
	std::cerr << std::endl;

	GxsMsgRelatedDataMap msgData;
	bool ok = RsGenExchange::getMsgRelatedData(token, msgData);
			
	if(ok)
	{
		GxsMsgRelatedDataMap::iterator mit = msgData.begin();
		
		for(; mit != msgData.end();  mit++)
		{
			std::vector<RsGxsMsgItem*>& msgItems = mit->second;
			std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();
			
			for(; vit != msgItems.end(); vit++)
			{
				RsGxsChannelPostItem* item = dynamic_cast<RsGxsChannelPostItem*>(*vit);
		
				if(item)
				{
					RsGxsChannelPost msg;
					item->toChannelPost(msg, true);
					msgs.push_back(msg);
					delete item;
				}
				else
				{
					std::cerr << "Not a GxsChannelPostItem, deleting!" << std::endl;
					delete *vit;
				}
			}
		}
	}
	else
	{
		std::cerr << "p3GxsChannels::getRelatedPosts() ERROR in request";
		std::cerr << std::endl;
	}
			
	return ok;
}


/********************************************************************************************/
/********************************************************************************************/

void p3GxsChannels::setChannelAutoDownload(uint32_t&, const RsGxsGroupId&, bool)
{
	std::cerr << "p3GxsChannels::setChannelAutoDownload() TODO";
	std::cerr << std::endl;

	return;
}


void p3GxsChannels::request_AllSubscribedGroups()
{
#ifdef GXSCHANNELS_DEBUG
        std::cerr << "p3GxsChannels::request_SubscribedGroups()";
        std::cerr << std::endl;
#endif // GXSCHANNELS_DEBUG

        uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

        uint32_t token = 0;

        RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts);
        GxsTokenQueue::queueRequest(token, GXSCHANNELS_SUBSCRIBED_META);

}


void p3GxsChannels::request_SpecificSubscribedGroups(const std::list<RsGxsGroupId> &groups)
{
#ifdef GXSCHANNELS_DEBUG
        std::cerr << "p3GxsChannels::request_SpecificSubscribedGroups()";
        std::cerr << std::endl;
#endif // GXSCHANNELS_DEBUG

        uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

        uint32_t token = 0;

        RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts, groups);
        GxsTokenQueue::queueRequest(token, GXSCHANNELS_SUBSCRIBED_META);
}


void p3GxsChannels::load_SubscribedGroups(const uint32_t &token)
{
#ifdef GXSCHANNELS_DEBUG
        std::cerr << "p3GxsChannels::load_SubscribedGroups()";
        std::cerr << std::endl;
#endif // GXSCHANNELS_DEBUG

	std::list<RsGroupMetaData> groups;
	std::list<RsGxsGroupId> groupList;

	getGroupMeta(token, groups);

	std::list<RsGroupMetaData>::iterator it;
	for(it = groups.begin(); it != groups.end(); it++)
	{
		if (it->mSubscribeFlags & 
			(GXS_SERV::GROUP_SUBSCRIBE_ADMIN |
			GXS_SERV::GROUP_SUBSCRIBE_PUBLISH |
			GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED ))
		{
        		std::cerr << "p3GxsChannels::load_SubscribedGroups() updating Subscribed Group: " << it->mGroupId;
        		std::cerr << std::endl;

			updateSubscribedGroup(*it);
			if (autoDownloadEnabled(it->mGroupId))
			{
        			std::cerr << "p3GxsChannels::load_SubscribedGroups() remembering AutoDownload Group: " << it->mGroupId;
        			std::cerr << std::endl;
				groupList.push_back(it->mGroupId);
			}
		}
		else
		{
        		std::cerr << "p3GxsChannels::load_SubscribedGroups() clearing unsubscribed Group: " << it->mGroupId;
        		std::cerr << std::endl;
			clearUnsubscribedGroup(it->mGroupId);
		}
	}

	/* Query for UNPROCESSED POSTS from checkGroupList */
	request_GroupUnprocessedPosts(groupList);
}



void p3GxsChannels::updateSubscribedGroup(const RsGroupMetaData &group)
{
        std::cerr << "p3GxsChannels::updateSubscribedGroup() id: " << group.mGroupId;
        std::cerr << std::endl;

	mSubscribedGroups[group.mGroupId] = group;
}


void p3GxsChannels::clearUnsubscribedGroup(const RsGxsGroupId &id)
{
        std::cerr << "p3GxsChannels::clearUnsubscribedGroup() id: " << id;
        std::cerr << std::endl;

	//std::map<RsGxsGroupId, RsGrpMetaData> mSubscribedGroups;
	std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;

	it = mSubscribedGroups.find(id);
	if (it != mSubscribedGroups.end())
	{
		mSubscribedGroups.erase(it);
	}
}


void p3GxsChannels::subscribeToGroup(const RsGxsGroupId &groupId, bool subscribe)
{
        std::cerr << "p3GxsChannels::subscribedToGroup() id: " << groupId << " subscribe: " << subscribe;
        std::cerr << std::endl;

	std::list<RsGxsGroupId> groups;
	groups.push_back(groupId);

	// Call down to do the real work.
	uint32_t token;
	RsGenExchange::subscribeToGroup(token, groupId, subscribe);

	// reload Group afterwards.
	request_SpecificSubscribedGroups(groups);
}


void p3GxsChannels::request_SpecificUnprocessedPosts(std::list<std::pair<RsGxsGroupId, RsGxsMessageId> > &ids)
{
#ifdef GXSCHANNELS_DEBUG
        std::cerr << "p3GxsChannels::request_SpecificUnprocessedPosts()";
        std::cerr << std::endl;
#endif // GXSCHANNELS_DEBUG

        uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA;
        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

        uint32_t token = 0;

        RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts);
        GxsTokenQueue::queueRequest(token, GXSCHANNELS_UNPROCESSED_SPECIFIC);
}


void p3GxsChannels::request_GroupUnprocessedPosts(const std::list<RsGxsGroupId> &grouplist)
{
#ifdef GXSCHANNELS_DEBUG
        std::cerr << "p3GxsChannels::request_GroupUnprocessedPosts()";
        std::cerr << std::endl;
#endif // GXSCHANNELS_DEBUG

        uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA;
        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

        uint32_t token = 0;

        RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts);
        GxsTokenQueue::queueRequest(token, GXSCHANNELS_UNPROCESSED_GENERIC);
}


void p3GxsChannels::load_SpecificUnprocessedPosts(const uint32_t &token)
{
	std::vector<RsGxsChannelPost> posts;
	if (!getRelatedPosts(token, posts))
	{
		std::cerr << "p3GxsChannels::load_GroupUnprocessedPosts ERROR";
		return;
	}


	std::vector<RsGxsChannelPost>::iterator it;
	for(it = posts.begin(); it != posts.end(); it++)
	{
		/* autodownload the files */
		handleUnprocessedPost(*it);
	}
}

	
void p3GxsChannels::load_GroupUnprocessedPosts(const uint32_t &token)
{
	std::vector<RsGxsChannelPost> posts;

	if (!getPostData(token, posts))
	{
		std::cerr << "p3GxsChannels::load_GroupUnprocessedPosts ERROR";
		return;
	}


	std::vector<RsGxsChannelPost>::iterator it;
	for(it = posts.begin(); it != posts.end(); it++)
	{
		handleUnprocessedPost(*it);
	}
}

	
	
void p3GxsChannels::handleUnprocessedPost(const RsGxsChannelPost &msg)
{
        std::cerr << "p3GxsChannels::handleUnprocessedPost() GroupId: " << msg.mMeta.mGroupId << " MsgId: " << msg.mMeta.mMsgId;
        std::cerr << std::endl;

	/* check that autodownload is set */
	if (autoDownloadEnabled(msg.mMeta.mGroupId))
	{
		
        	std::cerr << "p3GxsChannels::handleUnprocessedPost() AutoDownload Enabled ... handling";
        	std::cerr << std::endl;

		/* check the date is not too old */

		/* start download */

        	std::cerr << "p3GxsChannels::handleUnprocessedPost() START DOWNLOAD (TODO)";
        	std::cerr << std::endl;


		/* mark as processed */
		uint32_t token;
		RsGxsGrpMsgIdPair msgId(msg.mMeta.mGroupId, msg.mMeta.mMsgId);
		setMessageProcessedStatus(token, msgId, true);
	}
	else
	{
        	std::cerr << "p3GxsChannels::handleUnprocessedPost() AutoDownload Disabled ... skipping";
        	std::cerr << std::endl;
	}
}


        // Overloaded from GxsTokenQueue for Request callbacks.
void p3GxsChannels::handleResponse(uint32_t token, uint32_t req_type)
{
#ifdef GXSCHANNELS_DEBUG
        std::cerr << "p3GxsChannels::handleResponse(" << token << "," << req_type << ")";
        std::cerr << std::endl;
#endif // GXSCHANNELS_DEBUG

        // stuff.
        switch(req_type)
        {
        	case GXSCHANNELS_SUBSCRIBED_META:
			load_SubscribedGroups(token);
                        break;

		case GXSCHANNELS_UNPROCESSED_SPECIFIC:
			load_SpecificUnprocessedPosts(token);
                        break;

		case GXSCHANNELS_UNPROCESSED_GENERIC:
			load_SpecificUnprocessedPosts(token);
                        break;

                default:
                        /* error */
                        std::cerr << "p3GxsService::handleResponse() Unknown Request Type: " << req_type;
                        std::cerr << std::endl;
                        break;
        }
}


/********************************************************************************************/
/********************************************************************************************/


bool p3GxsChannels::autoDownloadEnabled(const RsGxsGroupId &id)
{
        std::cerr << "p3GxsChannels::autoDownloadEnabled(" << id << ")";
        std::cerr << std::endl;

	std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;

	it = mSubscribedGroups.find(id);
	if (it != mSubscribedGroups.end())
	{
        	std::cerr << "p3GxsChannels::autoDownloadEnabled() No Entry";
        	std::cerr << std::endl;

		return false;
	}

	/* extract from ServiceString */
	SSGxsChannelGroup ss;
	ss.load(it->second.mServiceString);
	return ss.mAutoDownload;
}

#define RSGXSCHANNEL_MAX_SERVICE_STRING	128

bool SSGxsChannelGroup::load(const std::string &input)
{
        char line[RSGXSCHANNEL_MAX_SERVICE_STRING];
	int download_val;
        mAutoDownload = false;
        if (1 == sscanf(input.c_str(), "D:%d", &download_val))
        {
		if (download_val == 1)
		{
                	mAutoDownload = true;
		}
        }
        return true;
}

std::string SSGxsChannelGroup::save() const
{
        std::string output;
        if (mAutoDownload)
        {
                output += "D:1";
        }
        else
        {
                output += "D:0";
        }
        return output;
}

bool p3GxsChannels::setAutoDownload(const RsGxsGroupId &groupId, bool enabled)
{
        std::cerr << "p3GxsChannels::setAutoDownload() id: " << groupId << " enabled: " << enabled;
        std::cerr << std::endl;

	std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;

	it = mSubscribedGroups.find(groupId);
	if (it != mSubscribedGroups.end())
	{
        	std::cerr << "p3GxsChannels::setAutoDownload() Missing Group";
        	std::cerr << std::endl;

		return false;
	}

	/* extract from ServiceString */
	SSGxsChannelGroup ss;
	ss.load(it->second.mServiceString);
	if (enabled == ss.mAutoDownload)
	{
		/* it should be okay! */
        	std::cerr << "p3GxsChannels::setAutoDownload() WARNING setting looks okay already";
        	std::cerr << std::endl;

	}

	/* we are just going to set it anyway. */
	ss.mAutoDownload = enabled;
	std::string serviceString = ss.save();
	uint32_t token;
	RsGenExchange::setGroupServiceString(token, groupId, serviceString);

	/* now reload it */
	std::list<RsGxsGroupId> groups;
	groups.push_back(groupId);

	request_SpecificSubscribedGroups(groups);

	return true;
}

/********************************************************************************************/
/********************************************************************************************/

void p3GxsChannels::setMessageProcessedStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool processed)
{
        std::cerr << "p3GxsChannels::setMessageProcessedStatus()";
        std::cerr << std::endl;

        uint32_t mask = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
        uint32_t status = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
        if (processed)
        {
                status = 0;
        }
        setMsgStatusFlags(token, msgId, status, mask);
}

void p3GxsChannels::setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read)
{
        std::cerr << "p3GxsChannels::setMessageReadStatus()";
        std::cerr << std::endl;

        uint32_t mask = GXS_SERV::GXS_MSG_STATUS_UNREAD;
        uint32_t status = GXS_SERV::GXS_MSG_STATUS_UNREAD;
        if (read)
        {
                status = 0;
        }
        setMsgStatusFlags(token, msgId, status, mask);
}


/********************************************************************************************/
/********************************************************************************************/

bool p3GxsChannels::createGroup(uint32_t &token, RsGxsChannelGroup &group)
{
	std::cerr << "p3GxsChannels::createGroup()" << std::endl;

	RsGxsChannelGroupItem* grpItem = new RsGxsChannelGroupItem();
	grpItem->fromChannelGroup(group, true);

	RsGenExchange::publishGroup(token, grpItem);
	return true;
}


bool p3GxsChannels::createPost(uint32_t &token, RsGxsChannelPost &msg)
{
	std::cerr << "p3GxsChannels::createChannelPost() GroupId: " << msg.mMeta.mGroupId;
	std::cerr << std::endl;

	RsGxsChannelPostItem* msgItem = new RsGxsChannelPostItem();
	msgItem->fromChannelPost(msg, true);
	
	RsGenExchange::publishMsg(token, msgItem);
	return true;
}


/********************************************************************************************/
/********************************************************************************************/

/* so we need the same tick idea as wiki for generating dummy channels
 */

#define 	MAX_GEN_GROUPS		5
#define 	MAX_GEN_MESSAGES	100

std::string p3GxsChannels::genRandomId()
{
        std::string randomId;
        for(int i = 0; i < 20; i++)
        {
                randomId += (char) ('a' + (RSRandom::random_u32() % 26));
        }

        return randomId;
}

bool p3GxsChannels::generateDummyData()
{
	mGenCount = 0;
	mGenRefs.resize(MAX_GEN_MESSAGES);

	std::string groupName;
	rs_sprintf(groupName, "TestChannel_%d", mGenCount);

	std::cerr << "p3GxsChannels::generateDummyData() Starting off with Group: " << groupName;
	std::cerr << std::endl;

	/* create a new group */
	generateGroup(mGenToken, groupName);

	mGenActive = true;

	return true;
}


void p3GxsChannels::dummy_tick()
{
	/* check for a new callback */

	if (mGenActive)
	{
		std::cerr << "p3GxsChannels::dummyTick() AboutActive";
		std::cerr << std::endl;

		uint32_t status = RsGenExchange::getTokenService()->requestStatus(mGenToken);
		if (status != RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
		{
			std::cerr << "p3GxsChannels::dummy_tick() Status: " << status;
			std::cerr << std::endl;

			if (status == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED)
			{
				std::cerr << "p3GxsChannels::dummy_tick() generateDummyMsgs() FAILED";
				std::cerr << std::endl;
				mGenActive = false;
			}
			return;
		}

		if (mGenCount < MAX_GEN_GROUPS)
		{
			/* get the group Id */
			RsGxsGroupId groupId;
			RsGxsMessageId emptyId;
			if (!acknowledgeTokenGrp(mGenToken, groupId))
			{
				std::cerr << " ERROR ";
				std::cerr << std::endl;
				mGenActive = false;
				return;
			}

			std::cerr << "p3GxsChannels::dummy_tick() Acknowledged GroupId: " << groupId;
			std::cerr << std::endl;

			ChannelDummyRef ref(groupId, emptyId, emptyId);
			mGenRefs[mGenCount] = ref;
		}
		else if (mGenCount < MAX_GEN_MESSAGES)
		{
			/* get the msg Id, and generate next snapshot */
			RsGxsGrpMsgIdPair msgId;
			if (!acknowledgeTokenMsg(mGenToken, msgId))
			{
				std::cerr << " ERROR ";
				std::cerr << std::endl;
				mGenActive = false;
				return;
			}

			std::cerr << "p3GxsChannels::dummy_tick() Acknowledged <GroupId: " << msgId.first << ", MsgId: " << msgId.second << ">";
			std::cerr << std::endl;

			/* store results for later selection */

			ChannelDummyRef ref(msgId.first, mGenThreadId, msgId.second);
			mGenRefs[mGenCount] = ref;
		}
		else
		{
			std::cerr << "p3GxsChannels::dummy_tick() Finished";
			std::cerr << std::endl;

			/* done */
			mGenActive = false;
			return;
		}

		mGenCount++;

		if (mGenCount < MAX_GEN_GROUPS)
		{
			std::string groupName;
			rs_sprintf(groupName, "TestChannel_%d", mGenCount);

			std::cerr << "p3GxsChannels::dummy_tick() Generating Group: " << groupName;
			std::cerr << std::endl;

			/* create a new group */
			generateGroup(mGenToken, groupName);
		}
		else
		{
			/* create a new message */
			uint32_t idx = (uint32_t) (mGenCount * RSRandom::random_f32());
			ChannelDummyRef &ref = mGenRefs[idx];

			RsGxsGroupId grpId = ref.mGroupId;
			RsGxsMessageId parentId = ref.mMsgId;
			mGenThreadId = ref.mThreadId;
			if (mGenThreadId.empty())
			{
				mGenThreadId = parentId;
			}

			std::cerr << "p3GxsChannels::dummy_tick() Generating Msg ... ";
			std::cerr << " GroupId: " << grpId;
			std::cerr << " ThreadId: " << mGenThreadId;
			std::cerr << " ParentId: " << parentId;
			std::cerr << std::endl;

			if (parentId.empty())
			{
				generatePost(mGenToken, grpId);
			}
			else
			{
				generateComment(mGenToken, grpId, parentId, mGenThreadId);
			}
		}
	}
}


bool p3GxsChannels::generatePost(uint32_t &token, const RsGxsGroupId &grpId)
{
	RsGxsChannelPost msg;

	std::string rndId = genRandomId();

	rs_sprintf(msg.mMsg, "Channel Msg: GroupId: %s, some randomness: %s", 
		grpId.c_str(), rndId.c_str());
	
	msg.mMeta.mMsgName = msg.mMsg;

	msg.mMeta.mGroupId = grpId;
	msg.mMeta.mThreadId = "";
	msg.mMeta.mParentId = "";

	msg.mMeta.mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED | GXS_SERV::GXS_MSG_STATUS_UNREAD;

	createPost(token, msg);

	return true;
}


bool p3GxsChannels::generateComment(uint32_t &token, const RsGxsGroupId &grpId, const RsGxsMessageId &parentId, const RsGxsMessageId &threadId)
{
	RsGxsComment msg;

	std::string rndId = genRandomId();

	rs_sprintf(msg.mComment, "Channel Comment: GroupId: %s, ThreadId: %s, ParentId: %s + some randomness: %s", 
		grpId.c_str(), threadId.c_str(), parentId.c_str(), rndId.c_str());
	
	msg.mMeta.mMsgName = msg.mComment;

	msg.mMeta.mGroupId = grpId;
	msg.mMeta.mThreadId = threadId;
	msg.mMeta.mParentId = parentId;

	msg.mMeta.mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED | GXS_SERV::GXS_MSG_STATUS_UNREAD;

	/* chose a random Id to sign with */
	std::list<RsGxsId> ownIds;
	std::list<RsGxsId>::iterator it;

	rsIdentity->getOwnIds(ownIds);

	uint32_t idx = (uint32_t) (ownIds.size() * RSRandom::random_f32());
	uint32_t i = 0;
	for(it = ownIds.begin(); (it != ownIds.end()) && (i < idx); it++, i++);

	if (it != ownIds.end())
	{
		std::cerr << "p3GxsChannels::generateMessage() Author: " << *it;
		std::cerr << std::endl;
		msg.mMeta.mAuthorId = *it;
	} 
	else
	{
		std::cerr << "p3GxsChannels::generateMessage() No Author!";
		std::cerr << std::endl;
	} 

	createComment(token, msg);

	return true;
}


bool p3GxsChannels::generateGroup(uint32_t &token, std::string groupName)
{
	/* generate a new channel */
	RsGxsChannelGroup channel;
	channel.mMeta.mGroupName = groupName;

	createGroup(token, channel);

	return true;
}


        // Overloaded from RsTickEvent for Event callbacks.
void p3GxsChannels::handle_event(uint32_t event_type, const std::string &elabel)
{
	std::cerr << "p3GxsChannels::handle_event(" << event_type << ")";
	std::cerr << std::endl;

	// stuff.
	switch(event_type)
	{
		case CHANNEL_TESTEVENT_DUMMYDATA:
			generateDummyData();
			break;

		case CHANNEL_PROCESS:
			request_AllSubscribedGroups();

		default:
			/* error */
			std::cerr << "p3GxsChannels::handle_event() Unknown Event Type: " << event_type;
			std::cerr << std::endl;
			break;
	}
}

