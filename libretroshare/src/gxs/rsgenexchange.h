#ifndef RSGENEXCHANGE_H
#define RSGENEXCHANGE_H

/*
 * libretroshare/src/gxs: rsgenexchange.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Christopher Evi-Parker, Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
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

#include <queue>
#include <ctime>

#include "rsgxs.h"
#include "rsgds.h"
#include "rsnxs.h"
#include "rsgxsdataaccess.h"
#include "rsnxsobserver.h"
#include "retroshare/rsgxsservice.h"
#include "serialiser/rsnxsitems.h"

typedef std::map<RsGxsGroupId, std::vector<RsGxsMsgItem*> > GxsMsgDataMap;
typedef std::map<RsGxsGroupId, RsGxsGrpItem*> GxsGroupDataMap;
typedef std::map<RsGxsGroupId, std::vector<RsMsgMetaData> > GxsMsgMetaMap;


/*!
 * This should form the parent class to \n
 * all gxs services. This provides access to service's msg/grp data \n
 * management/publishing/sync features
 *
 * Features: \n
 *         a. Data Access: \n
 *              Provided by handle to RsTokenService. This ensure consistency \n
 *              of requests and hiearchy of groups -> then messages which are \n
 *              sectioned by group ids. \n
 *              The one caveat is that redemption of tokens are done through \n
 *              the backend of this class \n
 *         b. Publishing: \n
 *              Methods are provided to publish msg and group items and also make \n
 *              changes to meta information of both item types \n
 *         c. Sync/Notification: \n
 *              Also notifications are made here on receipt of new data from \n
 *              connected peers
 */

class RsGixs;

class RsGenExchange : public RsGxsService, public RsNxsObserver
{
public:

    /*!
     * Constructs a RsGenExchange object, the owner ship of gds, ns, and serviceserialiser passes \n
     * onto the constructed object
     * @param gds Data service needed to act as store of message
     * @param ns Network service needed to synchronise data with rs peers
     * @param serviceSerialiser The users service needs this \n
     *        in order for gen exchange to deal with its data types
     * @param mServType This should be service type used by the serialiser
     * @param This is used for verification of msgs and groups received by Gen Exchange using identities, set to NULL if \n
     *        identity verification is not wanted
     */
    RsGenExchange(RsGeneralDataService* gds, RsNetworkExchangeService* ns, RsSerialType* serviceSerialiser, uint16_t mServType, RsGixs* gixs = NULL);

    virtual ~RsGenExchange();

    /** S: Observer implementation **/

    /*!
     * @param messages messages are deleted after function returns
     */
    void notifyNewMessages(std::vector<RsNxsMsg*>& messages);

    /*!
     * @param messages messages are deleted after function returns
     */
    void notifyNewGroups(std::vector<RsNxsGrp*>& groups);

    /** E: Observer implementation **/

    /*!
     * This is called by Gxs service runner
     * periodically, use to implement non
     * blocking calls
     */
    void tick();

    /*!
     * Any backgroup processing needed by
     */
    virtual void service_tick() = 0;


    /*!
     *
     * @return handle to token service handle for making
     * request to this gxs service
     */
    RsTokenService* getTokenService();

public:

    /** data access functions **/

    /*!
     * Retrieve group list for a given token
     * @param token
     * @param groupIds
     * @return false if token cannot be redeemed, if false you may have tried to redeem when not ready
     */
    bool getGroupList(const uint32_t &token, std::list<RsGxsGroupId> &groupIds);

    /*!
     * Retrieve msg list for a given token sectioned by group Ids
     * @param token token to be redeemed
     * @param msgIds a map of grpId -> msgList (vector)
     */
    bool getMsgList(const uint32_t &token, GxsMsgIdResult &msgIds);


    /*!
     * retrieve group meta data associated to a request token
     * @param token
     * @param groupInfo
     */
    bool getGroupMeta(const uint32_t &token, std::list<RsGroupMetaData> &groupInfo);

    /*!
     * retrieves message meta data associated to a request token
     * @param token token to be redeemed
     * @param msgInfo the meta data to be retrieved for token store here
     */
    bool getMsgMeta(const uint32_t &token, GxsMsgMetaMap &msgInfo);




protected:

    /*!
     * @param grpItem
     * @deprecated only here to temporarily to testing
     */
    void createDummyGroup(RsGxsGrpItem* grpItem);

    /*!
     * retrieves group data associated to a request token
     * @param token token to be redeemed for grpitem retrieval
     * @param grpItem the items to be retrieved for token are stored here
     */
    bool getGroupData(const uint32_t &token, std::vector<RsGxsGrpItem*>& grpItem);

    /*!
     * retrieves message data associated to a request token
     * @param token token to be redeemed for message item retrieval
     * @param msgItems
     */
    bool getMsgData(const uint32_t &token, GxsMsgDataMap& msgItems);

    /*!
     * Assigns a token value to passed integer
     * The status of the token can still be queried from request status feature
     * @warning the token space is shared with RsGenExchange backend, so do not
     * modify tokens except does you have created by calling generatePublicToken()
     * @return  token
     */
    uint32_t generatePublicToken();

    /*!
     * Updates the status of associate token
     * @warning the token space is shared with RsGenExchange backend, so do not
     * modify tokens except does you have created by calling generatePublicToken()
     * @param token
     * @param status
     * @return false if token could not be found, true if token disposed of
     */
    bool updatePublicRequestStatus(const uint32_t &token, const uint32_t &status);

    /*!
     * This gets rid of a publicly issued token
     * @param token
     * @return false if token could not found, true if token is disposed of
     */
    bool disposeOfPublicToken(const uint32_t &token);

    /*!
     * This gives access to the data store which hold msgs and groups
     * for the service
     * @return Data store for retrieving msgs and groups
     */
    RsGeneralDataService* getDataStore();

    /*!
     * Retrieve keys for a given group, \n
     * call is blocking retrieval for underlying db
     * @warning under normal circumstance a service should not need this
     * @param grpId the id of the group to retrieve keys for
     * @param keys this is set to the retrieved keys
     * @return false if group does not exist or grpId is empty
     */
    bool getGroupKeys(const RsGxsGroupId& grpId, RsTlvSecurityKeySet& keySet);

public:

    /*!
     * This allows the client service to acknowledge that their msgs has \n
     * been created/modified and retrieve the create/modified msg ids
     * @param token the token related to modification/create request
     * @param msgIds map of grpid->msgIds of message created/modified
     * @return true if token exists false otherwise
     */
    bool acknowledgeTokenMsg(const uint32_t& token, RsGxsGrpMsgIdPair& msgId);

    /*!
         * This allows the client service to acknowledge that their grps has \n
	 * been created/modified and retrieve the create/modified grp ids
	 * @param token the token related to modification/create request
	 * @param msgIds vector of ids of groups created/modified
	 * @return true if token exists false otherwise
	 */
    bool acknowledgeTokenGrp(const uint32_t& token, RsGxsGroupId& grpId);

protected:

    /** Modifications **/

    /*!
     * Enables publication of a group item \n
     * If the item exists already this is simply versioned \n
     * This will induce a related change message \n
     * Ownership of item passes to this rsgenexchange \n
     * @param token
     * @param grpItem
     */
    void publishGroup(uint32_t& token, RsGxsGrpItem* grpItem);

    /*!
     * Enables publication of a message item \n
     * Setting mOrigMsgId meta member to blank \n
     * leads to this msg being an original msg \n
     * if mOrigMsgId is not blank the msgId then this msg is \n
     * considered a versioned msg \n
     * Ownership of item passes to this rsgenexchange
     * @param token
     * @param msgItem
     */
    void publishMsg(uint32_t& token, RsGxsMsgItem* msgItem);

public:

    /*!
     * sets the group subscribe flag
     * @param token this is set to token value associated to this request
     * @param
     */
    void setGroupSubscribeFlags(uint32_t& token, const RsGxsGroupId& grpId, const uint32_t& status, const uint32_t& mask);

    void setGroupStatusFlags(uint32_t& token, const RsGxsGroupId& grpId, const uint32_t& status, const uint32_t& mask);

    void setGroupServiceString(uint32_t& token, const RsGxsGroupId& grpId, const std::string& servString);

    void setMsgStatusFlags(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, const uint32_t& status, const uint32_t& mask);

    void setMsgServiceString(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, const std::string& servString );



protected:

    /** Notifications **/

    /*!
     * This confirms this class as an abstract one that \n
     * should not be instantiated \n
     * The deriving class should implement this function \n
     * as it is called by the backend GXS system to \n
     * update client of changes which should \n
     * instigate client to retrieve new content from the system
     * @param changes the changes that have occured to data held by this service
     */
    virtual void notifyChanges(std::vector<RsGxsNotify*>& changes) = 0;




private:

    void processRecvdData();

    void processRecvdMessages();

    void processRecvdGroups();

    void publishGrps();

    void publishMsgs();

    /*!
     * processes msg local meta changes
     */
    void processMsgMetaChanges();

    /*!
     * Processes group local meta changes
     */
    void processGrpMetaChanges();

    void createGroup(RsNxsGrp* grp);
    bool createMessage(RsNxsMsg* msg);


    /*!
     * check meta change is legal
     * @return false if meta change is not legal
     */
    bool locked_validateGrpMetaChange(GrpLocMetaData&);

private:

    RsMutex mGenMtx;
    RsGxsDataAccess* mDataAccess;
    RsGeneralDataService* mDataStore;
    RsNetworkExchangeService *mNetService;
    RsSerialType *mSerialiser;
    RsGixs* mGixs;

    std::vector<RsNxsMsg*> mReceivedMsgs;
    std::vector<RsNxsGrp*> mReceivedGrps;

    std::map<uint32_t, RsGxsGrpItem*> mGrpsToPublish;
    std::map<uint32_t, RsGxsMsgItem*> mMsgsToPublish;

    std::map<uint32_t, RsGxsGrpMsgIdPair > mMsgNotify;
    std::map<uint32_t, RsGxsGroupId> mGrpNotify;

    // for loc meta changes
    std::map<uint32_t, GrpLocMetaData > mGrpLocMetaMap;
    std::map<uint32_t,  MsgLocMetaData> mMsgLocMetaMap;

    std::vector<RsGxsNotify*> mNotifications;

    /// service type
    uint16_t mServType;


private:

    std::vector<RsGxsNotify*> mChanges;
};

#endif // RSGENEXCHANGE_H
