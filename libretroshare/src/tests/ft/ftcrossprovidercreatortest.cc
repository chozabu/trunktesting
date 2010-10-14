#include <stdexcept>
#include "ft/ftfilecreator.h"

#include "util/utest.h"
#include "util/rsdir.h"
#include <stdlib.h>

#include "util/rswin.h"
#include "ft/ftserver.h"

INITTEST();

static void createTmpFile(const std::string& name,uint64_t size,std::string& hash) ;
static void transfer(ftFileProvider *server, const std::string& server_peer_id,ftFileCreator *client,const std::string& client_peer_id) ;

int main()
{
	// We create two file creators and feed them with a file both from elsewhere and from each others, as file providers.
	// This test should check that no data race occurs while reading/writting data and exchanging chunks.
	//

	// 1 - Create a temporary file, compute its hash
	//
	std::string fname = "source_tmp.bin" ;
	std::string fname_copy_1 = "copy_1_tmp.bin" ;
	std::string fname_copy_2 = "copy_2_tmp.bin" ;
	uint64_t size = 55766278 ;
	std::string hash = "" ;

	createTmpFile(fname,size,hash) ;

	// 2 - Create one file provider for this file, and 2 file creators
	//

	ftFileProvider *server = new ftFileProvider(fname,size,hash) ;
	ftFileCreator *client1 = new ftFileCreator(fname_copy_1,size,hash) ;
	ftFileCreator *client2 = new ftFileCreator(fname_copy_2,size,hash) ;

	// 3 - Exchange chunks, and build two copies of the file.
	//
	std::string peer_id_1("client peer id 1") ;
	std::string peer_id_2("client peer id 2") ;
	std::string server_id("server peer id") ;

	while( !(client1->finished() && client2->finished()))
	{
		// 1 - select a random client, get the data from him.

		ftFileProvider *tmpserver ;
		ftFileCreator  *tmpclient ;
		std::string tmpserver_pid ;
		std::string tmpclient_pid ;
		
		// choose client and server, randomly
		//
		if(lrand48()&1)
		{
			tmpclient = client1 ;
			tmpclient_pid = peer_id_1 ;

			if(lrand48()&1)
			{
				tmpserver = client2 ;
				tmpserver_pid = peer_id_2 ;
			}
			else
			{
				tmpserver = server ;
				tmpserver_pid = server_id ;
			}
		}
		else
		{
			tmpclient = client2 ;
			tmpclient_pid = peer_id_2 ;
			
			if(lrand48()&1)
			{
				tmpserver = client1 ;
				tmpserver_pid = peer_id_1 ;
			}
			else
			{
				tmpserver = server ;
				tmpserver_pid = server_id ;
			}

		}

		// 2 - transfer from the server to the client.

		transfer(tmpserver,tmpserver_pid,tmpclient,tmpclient_pid) ;
	}

	// hash the received data
	
	std::string hash1 ; client1->hashReceivedData(hash1) ;
	std::string hash2 ; client2->hashReceivedData(hash2) ;

	std::cout << "Hash  = " << hash  << std::endl ;
	std::cout << "Hash1 = " << hash1 << std::endl ;
	std::cout << "Hash2 = " << hash2 << std::endl ;
	
	if(hash  != hash1) FAILED("hashs 0/1 defer after transfer !") ;
	if(hash  != hash2) FAILED("hashs 0/2 defer after transfer !") ;
	if(hash1 != hash2) FAILED("hashs 1/2 defer after transfer !") ;

	FINALREPORT("File transfer test");

	return TESTRESULT();
}

void transfer(ftFileProvider *server, const std::string& server_peer_id,ftFileCreator *client,const std::string& client_peer_id)
{
	uint32_t size_hint = 128 + (lrand48()%8000) ;
	uint64_t offset = 0;
	uint32_t chunk_size = 0;
	bool toOld = false ;

	if(! client->getMissingChunk(server_peer_id, size_hint, offset, chunk_size, toOld))
		return ;

	void *data = malloc(chunk_size) ;

	if( server->getFileData(client_peer_id,offset,chunk_size,data) )
		client->addFileData(offset,chunk_size,data) ;

	free(data) ;
}

void createTmpFile(const std::string& name,uint64_t S,std::string& hash)
{
	FILE *tmpf = fopen("source_tmp.bin","w") ;

	// write 1MB chunks
	uint64_t C = 1024*1024 ;
	uint8_t *data = new uint8_t[C] ;

	for(uint i=0;i<=S/C;++i)
	{
		int Cp = C ;
		if(i==S/C)
		{
			if(S%C>0)
				Cp = S%C ;
			else
				break ;
		}

		for(int k=0;k<Cp;++k)
			data[k] = lrand48()%256 ;

		size_t n = fwrite(data,1,Cp,tmpf) ;
		if(n != Cp)
		{
			std::cerr << "Wrote " << n << " expected " << Cp << std::endl;
			throw std::runtime_error("Could not write file !") ;
		}
	}
	fclose(tmpf) ;

	RsDirUtil::hashFile(name,hash) ;
}
