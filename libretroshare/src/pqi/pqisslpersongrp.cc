/*
 * libretroshare/src/pqi: pqisslpersongrp.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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

#include "serialiser/rsserviceserialiser.h"
#include "util/rsdebug.h"

#include "pqi/pqisslpersongrp.h"
#include "pqi/authssl.h"


const int pqipersongrpzone = 354;

/****
 * #define PQI_DISABLE_UDP 1
 ***/

/********************************** SSL Specific features ***************************/

#include "pqi/pqissl.h"
#include "pqi/pqissllistener.h"
#include "pqi/p3peermgr.h"


#ifndef PQI_DISABLE_UDP
  #include "pqi/pqissludp.h"
#endif

#include "pqi/pqisslproxy.h"

pqilistener * pqisslpersongrp::locked_createListener(const struct sockaddr_storage &laddr)
{
	pqilistener *listener = new pqissllistener(laddr, mPeerMgr);
	return listener;
}

pqiperson * pqisslpersongrp::locked_createPerson(std::string id, pqilistener *listener)
{
	std::cerr << "pqisslpersongrp::locked_createPerson() PeerId: " << id;
	std::cerr << std::endl;

	pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, "pqipersongrp::createPerson() PeerId: " + id);

	pqiperson *pqip = new pqiperson(id, this);

	// If using proxy, then only create a proxy item, otherwise can use any.
	// If we are a hidden node - then all connections should be via proxy.
	if (mPeerMgr->isHiddenPeer(id) || mPeerMgr->isHidden())
	{
		std::cerr << "pqisslpersongrp::locked_createPerson() Is Hidden Peer!";
		std::cerr << std::endl;

		pqisslproxy *pqis   = new pqisslproxy((pqissllistener *) listener, pqip, mLinkMgr);
	
		/* construct the serialiser ....
		 * Needs:
		 * * FileItem
		 * * FileData
		 * * ServiceGeneric
		 */

	
		RsSerialiser *rss = new RsSerialiser();
		rss->addSerialType(new RsServiceSerialiser());
	
		pqiconnect *pqisc = new pqiconnect(pqip, rss, pqis);
	
		pqip -> addChildInterface(PQI_CONNECT_HIDDEN_TCP, pqisc);
	}
	else
	{	
		std::cerr << "pqisslpersongrp::locked_createPerson() Is Normal Peer!";
		std::cerr << std::endl;

		pqissl *pqis   = new pqissl((pqissllistener *) listener, pqip, mLinkMgr);
	
		/* construct the serialiser ....
		 * Needs:
		 * * FileItem
		 * * FileData
		 * * ServiceGeneric
		 */
	
		ssl_tunnels[id] = pqis ;	// keeps for getting crypt info per peer.
	
		RsSerialiser *rss = new RsSerialiser();
		rss->addSerialType(new RsServiceSerialiser());
	
		pqiconnect *pqisc = new pqiconnect(pqip, rss, pqis);
	
		pqip -> addChildInterface(PQI_CONNECT_TCP, pqisc);
	
#ifndef PQI_DISABLE_UDP
		pqissludp *pqius 	= new pqissludp(pqip, mLinkMgr);
	
		RsSerialiser *rss2 = new RsSerialiser();
		rss2->addSerialType(new RsServiceSerialiser());
		
		pqiconnect *pqiusc 	= new pqiconnect(pqip, rss2, pqius);
	
		// add a ssl + proxy interface.
		// Add Proxy First.
		pqip -> addChildInterface(PQI_CONNECT_UDP, pqiusc);
#endif
	}

	return pqip;
}


/********************************** SSL Specific features ***************************/


