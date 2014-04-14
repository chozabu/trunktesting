/*
 * Retroshare MeshDemoc Plugin.
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

#include <QDateTime>

#include "MeshDemocItem.h"
#include "gui/feeds/FeedHolder.h"

#include <retroshare/rsmeshdemoc.h>

#include <iostream>

/** Constructor */

MeshDemocItem::MeshDemocItem(FeedHolder *parent, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome)
	:GxsFeedItem(parent, feedId, groupId, messageId, isHome, rsMeshDemoc, true)
{
	setup();
}

MeshDemocItem::MeshDemocItem(FeedHolder *parent, uint32_t feedId, const RsMeshDemocPost &post, bool isHome)
	:GxsFeedItem(parent, feedId, post.mMeta.mGroupId, post.mMeta.mMsgId, isHome, rsMeshDemoc, false),
	mPost(post)
{
	setup();

	setContent(mPost);
}

void MeshDemocItem::setup()
{
	setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);

	connect(commentButton, SIGNAL( clicked()), this, SLOT(loadComments()));
	connect(voteUpButton, SIGNAL(clicked()), this, SLOT(makeUpVote()));
	connect(voteDownButton, SIGNAL(clicked()), this, SLOT( makeDownVote()));
	connect(votesButton, SIGNAL(clicked()), this, SLOT( showVotes()));
	connect(representButton, SIGNAL(clicked()), this, SLOT( selectRepr()));
}

void MeshDemocItem::loadMessage(const uint32_t &token)
{
	std::vector<RsMeshDemocPost> posts;
	if (!rsMeshDemoc->getPostData(token, posts))
	{
		std::cerr << "GxsChannelPostItem::loadMessage() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	if (posts.size() != 1)
	{
		std::cerr << "GxsChannelPostItem::loadMessage() Wrong number of Items";
		std::cerr << std::endl;
		return;
	}

	mPost = posts[0];
	setContent(mPost);
}
RsMeshDemocPost* MeshDemocItem::getContent()
{
	return &mPost;
}

void MeshDemocItem::setContent(const RsMeshDemocPost &post)
{
	mPost = post;

	QDateTime qtime;
	qtime.setTime_t(mPost.mMeta.mPublishTs);
	QString timestamp = qtime.toString("dd.MMMM yyyy hh:mm");
	dateLabel->setText(timestamp);
	fromLabel->setId(post.mMeta.mAuthorId);
	titleLabel->setText("<a href=" + QString::fromStdString(post.mLink) +
					   "><span style=\" text-decoration: underline; color:#2255AA;\">" +
					   QString::fromStdString(post.mMeta.mMsgName) + "</span></a>");
	siteLabel->setText("<a href=" + QString::fromStdString(post.mLink) +
					   "><span style=\" text-decoration: underline; color:#2255AA;\">" +
					   QString::fromStdString(post.mLink) + "</span></a>");

	//QString score = "Hot" + QString::number(post.mHotScore);
	//score += " Top" + QString::number(post.mTopScore); 
	//score += " New" + QString::number(post.mNewScore);

	QString score = QString::number(post.mTopScore);
	scoreLabel->setText(score); 

	QString dscore = QString::number(post.mDirectScore);
	directScoreLabel->setText(dscore);

	QString lscore = QString::number(post.mLiquidScore);
	liquidScoreLabel->setText(lscore);

	// FIX THIS UP LATER.
	notes->setText(QString::fromUtf8(post.mNotes.c_str()));
	// differences between Feed or Top of Comment.
	if (mParent)
	{
		// feed.
		frame_notes->hide();
		//frame_comment->show();
		commentButton->show();

		if (post.mComments)
		{
			QString commentText = QString::number(post.mComments);
			commentText += " ";
			commentText += tr("Comments");
			commentButton->setText(commentText);
		}
		else
		{
			commentButton->setText(tr("Comment"));
		}
	}
	else
	{
		// no feed.
		if(notes->text().isEmpty())
		{		
      frame_notes->hide();
		}
		else
		{
      frame_notes->show();
		}
		//frame_comment->hide();
		commentButton->hide();
	}

	// disable voting buttons - if they have already voted.
	if (post.mMeta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_MASK)
	{
		voteUpButton->setEnabled(false);
		voteDownButton->setEnabled(false);
	}

	uint32_t up, down, nComments;

#if 0
	bool ok = rsMeshDemoc->retrieveScores(mPost.mMeta.mServiceString, up, down, nComments);

	if(ok)
	{
		int32_t vote = up - down;
		scoreLabel->setText(QString::number(vote));

		numCommentsLabel->setText("<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px;"
								  "margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span"
								  "style=\" font-size:10pt; font-weight:600;\">#</span><span "
								  "style=\" font-size:8pt; font-weight:600;\"> Comments:  "
								  + QString::number(nComments) + "</span></p>");
	}
#endif
}

const RsMeshDemocPost &MeshDemocItem::getPost() const
{
	return mPost;
}

RsMeshDemocPost &MeshDemocItem::post()
{
	return mPost;
}

void MeshDemocItem::makeDownVote()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

	voteUpButton->setEnabled(false);
	voteDownButton->setEnabled(false);

	emit vote(msgId, false);
}

void MeshDemocItem::makeUpVote()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

	voteUpButton->setEnabled(false);
	voteDownButton->setEnabled(false);

	emit vote(msgId, true);
}
void MeshDemocItem::showVotes()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

	emit votesReq(msgId, mPost.mMeta.mMsgName);
}
void MeshDemocItem::selectRepr()
{
	emit reprReq(mPost.mMeta.mAuthorId);
}

void MeshDemocItem::loadComments()
{
	std::cerr << "MeshDemocItem::loadComments()";
	std::cerr << std::endl;
	if (mParent)
	{
		QString title = QString::fromUtf8(mPost.mMeta.mMsgName.c_str());
		mParent->openComments(0, mPost.mMeta.mGroupId, mPost.mMeta.mMsgId, title);
	}
}