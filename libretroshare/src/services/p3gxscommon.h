/*
 * libretroshare/src/services p3gxscommon.cc
 *
 * GxsChannels interface for RetroShare.
 *
 * Copyright 2012-2013 by Robert Fernie.
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
#ifndef P3_GXSCOMMON_SERVICE_HEADER
#define P3_GXSCOMMON_SERVICE_HEADER

#include "retroshare/rsgxscommon.h"
#include "gxs/rsgenexchange.h"
#include "gxs/gxstokenqueue.h"
#include <stdio.h>


/****
 * This mirrors rsGxsCommentService, with slightly different names, 
 * provides the implementation for any services requiring Comments.
 */

#
class VoteHolder
{
	public:

	static const uint32_t VOTE_ERROR     = 0;
	static const uint32_t VOTE_QUEUED    = 1;
	static const uint32_t VOTE_SUBMITTED = 2;
	static const uint32_t VOTE_READY     = 3;

	VoteHolder() :mVoteToken(0), mReqToken(0), mStatus(VOTE_ERROR) { return; }

	VoteHolder(const RsGxsVote &vote, uint32_t reqToken)
	:mVote(vote), mVoteToken(0), mReqToken(reqToken), mStatus(VOTE_QUEUED) { return; }

	RsGxsVote mVote;
	uint32_t  mVoteToken;
	uint32_t  mReqToken;
	uint32_t  mStatus;
};


class p3GxsCommentService: public GxsTokenQueue
{
	public:

	p3GxsCommentService(RsGenExchange *exchange, uint16_t service_type);

	void comment_tick();

	bool getGxsCommentData(const uint32_t &token, std::vector<RsGxsComment> &msgs);
	bool getGxsRelatedVotes(const uint32_t &token, std::multimap<RsGxsMessageId, RsGxsVote *> &voteMap);
	bool getGxsRelatedComments(const uint32_t &token, std::vector<RsGxsComment> &msgs);

	bool createGxsComment(uint32_t &token, RsGxsComment &msg);
	bool createGxsVote(uint32_t &token, RsGxsVote &msg);

	// Special Acknowledge.
    	bool acknowledgeVote(const uint32_t& token, RsGxsGrpMsgIdPair& msgId);

static double calculateBestScore(int upVotes, int downVotes);


#if 0
	void setGxsMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read);
#endif

	protected:

	// Overloaded from GxsTokenQueue for Request callbacks.
virtual void handleResponse(uint32_t token, uint32_t req_type);

	private:

	void load_PendingVoteParent(const uint32_t &token);
	void completeInternalVote(uint32_t &token);

	bool castVote(uint32_t &token, RsGxsVote &msg);

	RsGenExchange 	*mExchange;
	uint16_t	mServiceType;

	/* pending queue of Votes */
	std::map<RsGxsGrpMsgIdPair, VoteHolder> mPendingVotes;


};


#endif

