/*
   MBEnum   
   Copyright (c) 2003 Patrik Karlsson

   http://www.cqure.net

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>
#include <assert.h>
#include <windows.h> 
#include <lm.h>
#include "getopt.h"

char *pBanner = "MBEnum v.1.5.0 by patrik@cqure.net\n"
				"----------------------------------\n";

const int FILTER_NONE = 0;
const int FILTER_MASK = 1;
const int FILTER_HOST = 2;

struct t_server {
	LPSERVER_INFO_101 info;
	void *next;
};

struct t_filter {
	int filter_type;
	LPTSTR pHost;
	DWORD dwMask;
};


#define TYPES 19

char *pTypeNames[TYPES] = {"Server", "Workstation", "SQL Server",
						"Primary DC", "Backup DC", 
						"Timeserver", "Apple server", 
						"Novell server", "LM dom. member",
						"Print servers", "RAS server", \
						"Xenix server", "NW file & print",
						"Master browser",
						"Dom. M. browser",
						"Primary domain", "Term. server", "Member server", "Server systems" };

DWORD dwTypes[TYPES] = { SV_TYPE_SERVER, SV_TYPE_WORKSTATION, SV_TYPE_SQLSERVER,
						SV_TYPE_DOMAIN_CTRL, SV_TYPE_DOMAIN_BAKCTRL, SV_TYPE_TIME_SOURCE,
						SV_TYPE_AFP, SV_TYPE_NOVELL, SV_TYPE_DOMAIN_MEMBER,
						SV_TYPE_PRINTQ_SERVER, SV_TYPE_DIALIN_SERVER, SV_TYPE_XENIX_SERVER,
						SV_TYPE_SERVER_MFPN, SV_TYPE_MASTER_BROWSER, SV_TYPE_DOMAIN_MASTER,
						SV_TYPE_DOMAIN_ENUM, SV_TYPE_TERMINALSERVER, SV_TYPE_SERVER_NT,
						SV_TYPE_ALL & (SV_TYPE_SERVER_NT + SV_TYPE_DOMAIN_CTRL + SV_TYPE_DOMAIN_BAKCTRL) };



void outputTypes() {

	int i;

	for ( i=0; i<TYPES; i++ )
		fprintf(stderr, "%s\n", pTypeNames[i] );

}

struct t_server *addServer( struct t_server *pServer, LPSERVER_INFO_101 pInfo ) {

	struct t_server *pNewServer;

	pNewServer = (struct t_server *) malloc( sizeof( struct t_server ) );

	pNewServer->info = pInfo;

	pNewServer->next = pServer;
	pServer = pNewServer;

	return pServer;

}

void freeServers( struct t_server *pServer ) {

	struct t_server *rmv;
	struct t_server *curr;

	curr = pServer;

	while ( curr ) {
		rmv = curr;
		curr = curr->next;
		free(rmv);
	}

}


DWORD getServiceByName( char *pName ) {

	int nTypes = TYPES;
	DWORD dwType = 0;

	if ( strlen(pName) > 255 )
		return 0;

	do {

		nTypes --;
		
		if ( !stricmp( pName, pTypeNames[nTypes] ) )
			return dwTypes[nTypes];
	
	} while ( nTypes != 0 );

	return dwType;

}

void outputByServer( struct t_server *pServer, struct t_filter *pFilter ) {

	struct t_server *pServers = pServer;
	int i = 0;
	int nFirst = 0;

	while ( pServers ) {
		
		wprintf(L"Name: %s\n", pServers->info->sv101_name );
		wprintf(L"Services: ");

		nFirst = 0;

		/* Dont ouptut last TYPE */
		for ( i=0; i<TYPES-1; i ++ ) {
			
			if ( pServers->info->sv101_type & getServiceByName( pTypeNames[i] ) ) {
			
				if ( nFirst )
					printf(",");
				
				printf("%s", pTypeNames[i]);
				nFirst ++;
			}

		}
		wprintf(L"\n");
		wprintf(L"Comment: %s\n", pServers->info->sv101_comment );
		//wprintf(L"Type: " );

		wprintf(L"OS Version: ");

		if ( pServers->info->sv101_version_major == 4 &&
			 pServers->info->sv101_version_minor == 0 )
				wprintf(L"Windows NT 4.0");
		else if ( pServers->info->sv101_version_major == 4 &&
			 pServers->info->sv101_version_minor == 2 )
				wprintf(L"Samba Server");
		else if ( pServers->info->sv101_version_major == 5 &&
				  pServers->info->sv101_version_minor == 0 )
				wprintf(L"Windows 2000");
		else if ( pServers->info->sv101_version_major == 5 &&
				  pServers->info->sv101_version_minor == 1 )
				wprintf(L"Windows XP");
		else if ( pServers->info->sv101_version_major == 5 &&
				  pServers->info->sv101_version_minor == 2 )
				wprintf(L"Windows 2003");
		else
			wprintf(L"%d.%d", pServers->info->sv101_version_major, pServers->info->sv101_version_minor );

		/* Determine whether we're dealing with a workstation or server */
		if ( ( pServers->info->sv101_type & SV_TYPE_SERVER_NT ) ||
			 ( pServers->info->sv101_type & SV_TYPE_DOMAIN_CTRL ) ||
			 ( pServers->info->sv101_type & SV_TYPE_DOMAIN_BAKCTRL ) )

			wprintf(L" Server\n");
		else
			wprintf(L" Workstation\n");


		wprintf(L"\n\n");
		
		pServers = pServers->next;
	}

}

/*
	Code changed to only print those types that have servers
	registered
*/
void outputByService( struct t_server *pServer, const struct t_filter *pFilter ) {

	struct t_server *pServers = pServer;
	DWORD dwMask;
	int i;
	int nCommaCounter = 0;

	for ( i=0; i<TYPES; i ++ ) {
	
		dwMask = getServiceByName( pTypeNames[i] );

		if ( ( pFilter->filter_type == FILTER_NONE ) ||
			( ( pFilter->filter_type == FILTER_MASK ) && ( pFilter->dwMask == dwMask ) ) ||
			( ( pFilter->filter_type == FILTER_HOST ) ) ) {

			while ( pServers ) {
		
				if ( dwMask & pServers->info->sv101_type ) {
						
					if ( nCommaCounter == 0 )
						printf( "%s: ", pTypeNames[i] );
					else if ( nCommaCounter > 0 )
							printf(",");
					wprintf(L"%s", pServers->info->sv101_name );
					nCommaCounter ++;
				}

		
				pServers = pServers->next;

			}

		}

		if ( nCommaCounter > 0 )
			printf("\n");

		nCommaCounter = 0;
		pServers = pServer;

	}


	/* dirty solutions inc .....
	   Until I come up with a better way to enumerat workstations this has to stay :(
	*/
	while ( ( pFilter->dwMask != 0 ) && pServers ) {
	

		if ( !( SV_TYPE_ALL & (SV_TYPE_SERVER_NT + SV_TYPE_DOMAIN_CTRL + SV_TYPE_DOMAIN_BAKCTRL) & pServers->info->sv101_type ) ) {
				
			if ( nCommaCounter == 0 )
				printf( "Workstations: " );
			else if ( nCommaCounter > 0 )
					printf(",");
			wprintf(L"%s", pServers->info->sv101_name );
			nCommaCounter ++;
		}

		pServers = pServers->next;

	}

	if ( nCommaCounter > 0 )
		printf("\n");

	/* end dirty solutions inc ..... */

}

void outputByServiceVert( struct t_server *pServer, const struct t_filter *pFilter ) {

	struct t_server *pServers = pServer;
	DWORD dwMask;
	int i;
	int nCommaCounter = 0;

	for ( i=0; i<TYPES; i ++ ) {
	
		dwMask = getServiceByName( pTypeNames[i] );

		if ( ( pFilter->filter_type == FILTER_NONE ) ||
			( ( pFilter->filter_type == FILTER_MASK ) && ( pFilter->dwMask == dwMask ) ) ) {

			printf( "\n%s\n", pTypeNames[i] );

			while ( pServers ) {
		
				if ( dwMask & pServers->info->sv101_type ) {
					if ( nCommaCounter > 0 )
							printf("\n");
					wprintf(L"%s", pServers->info->sv101_name );
					nCommaCounter ++;
				}

				pServers = pServers->next;

			}

		}

		if ( nCommaCounter > 0 )
			printf("\n");

		nCommaCounter = 0;
		pServers = pServer;
		
	}


	/* dirty solutions inc .....
	   Until I come up with a better way to enumerat workstations this has to stay :(
	*/
	while ( pServers ) {
	

		if ( !( SV_TYPE_ALL & (SV_TYPE_SERVER_NT + SV_TYPE_DOMAIN_CTRL + SV_TYPE_DOMAIN_BAKCTRL) & pServers->info->sv101_type ) ) {
				
			if ( nCommaCounter == 0 )
				printf( "\nWorkstations\n" );
			else if ( nCommaCounter > 0 )
					printf("\n");
			wprintf(L"%s", pServers->info->sv101_name );
			nCommaCounter ++;
		}

		pServers = pServers->next;

	}

	if ( nCommaCounter > 0 )
		printf("\n");



}

void usage( char **argv ) {

	fprintf(stderr, "%s", pBanner);
	fprintf(stderr, "%s [-s \\\\server] [-d dom ] [-f filter] -p <mode>\n\n", argv[0] );
	fprintf(stderr, "Presentation modes:\n\n");
	fprintf(stderr, "1 - by server\n");
	fprintf(stderr, "2 - by service\n");
	fprintf(stderr, "3 - by service vertically\n");


}

int main(int argc, char **argv) {

	LPSERVER_INFO_101 pBuf = NULL;
	LPSERVER_INFO_101 pTmpBuf;
	DWORD dwLevel = 101;
	DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
	DWORD dwEntriesRead = 0;
	DWORD dwTotalEntries = 0;
	DWORD dwTotalCount = 0;
	DWORD dwServerType = SV_TYPE_ALL;//SV_TYPE_SERVER; // all servers
	DWORD dwResumeHandle = 0;
	NET_API_STATUS nStatus;
	LPTSTR pServerName = NULL;
	int nLen = 256;
	DWORD i;
	LPTSTR pszDomain = NULL;
	int c, nOrder = 0;

	struct t_server *pServer;
	struct t_filter filter;

	pServer = NULL;
	filter.dwMask = 0;
	filter.pHost = NULL;
	filter.filter_type = FILTER_NONE;

	while (1) {

		c = getopt (argc, argv, "s:p:f:hd:");

	    if ( c == -1 )
			break;

		switch (c) {

			case 'd':
				if ( strlen(optarg) > 128 ) {
					fprintf(stderr, "ERROR: Domain too long\n");
					exit(1);
				}

				pszDomain = malloc( nLen );

				if ( !pszDomain ) {
					fprintf(stderr, "ERROR: Failed to allocate %d bytes of memory\n", nLen);
					exit(1);
				}

				memset( pszDomain, 0, nLen );
				MultiByteToWideChar( CP_ACP, 0, optarg, -1, pszDomain, nLen );
				break;
      
			case 's':

				if ( strlen(optarg) > 128 ) {
					fprintf(stderr, "ERROR: Servername too long\n");
					exit(1);
				}

				pServerName = malloc( nLen );
				
				if ( !pServerName ) {
					fprintf(stderr, "ERROR: Failed to allocate %d bytes of memory\n", nLen);
					exit(1);
				}

				memset( pServerName, 0, nLen );
				MultiByteToWideChar( CP_ACP, 0, optarg, -1, pServerName, nLen );
				break;

			case 'g':
				
				if ( strlen(optarg) > 128 ) {
					fprintf(stderr, "ERROR: Hostname too long\n");
					exit(1);
				}

				filter.pHost = malloc( nLen );
				MultiByteToWideChar( CP_ACP, 0, optarg, -1, filter.pHost, nLen );

				filter.filter_type = FILTER_HOST;
				break;

			case 'p':
				nOrder = atoi( optarg );

				if ( nOrder < 1 || nOrder > 3 ) {
					fprintf(stderr, "ERROR: Unsupported presentation order\n");
					exit(1);
				}
				break;

			case 'f':
				
				/* Reset server time on first occurence of f parameter */
				if ( dwServerType == SV_TYPE_ALL ) {
					dwServerType = 0;
					filter.dwMask = 0;
				}

				/* in case of more f parameters expand the filter criteria */
				dwServerType += getServiceByName(optarg);

				filter.filter_type = FILTER_MASK;
				filter.dwMask += dwServerType;

				if ( dwServerType == 0 ) {
					fprintf(stderr, "ERROR: Unknown filter. Please choose one of the following:\n");
					outputTypes();
					exit(1);
				}
				break;

			case 'h':
				usage( argv );
				exit(1);

			default:
				usage( argv );
				exit(1);


		}

	}

	if ( nOrder == 0 ) {
		usage( argv );

		if ( !pServerName )
			free( pServerName );

		exit(1);
	}

	nStatus = NetServerEnum((const char *)pServerName, dwLevel, (LPBYTE *) &pBuf,
                           dwPrefMaxLen, &dwEntriesRead, &dwTotalEntries,
                           dwServerType, (const char *)pszDomain, &dwResumeHandle);

	if ( (nStatus == NERR_Success) || (nStatus == ERROR_MORE_DATA) ) {
	   
		if ((pTmpBuf = pBuf) != NULL) {
			
			for (i = 0; i < dwEntriesRead; i++) {

				if (pTmpBuf == NULL) {
					fprintf(stderr, "An access violation has occurred\n");
					break;
				}	

				pServer = addServer( pServer, pTmpBuf );

				pTmpBuf++;
				dwTotalCount++;
			}

			if (nStatus == ERROR_MORE_DATA) {
				fprintf(stderr, "\nMore entries available!!!\n");
				fprintf(stderr, "Total entries: %d", dwTotalEntries);
			}

      }


	}
	else {
		fprintf(stderr, "A system error has occurred: %d\n", nStatus);

		if ( !pServerName )
			free( pServerName );

		exit(1);
	}

	if ( !pServerName )
		free( pServerName );

	fprintf(stdout, "\n%s", pBanner );

	switch ( nOrder ) {
		
		case 1:
			outputByServer( pServer, &filter );
			break;
		case 2:
			outputByService( pServer, &filter );
			break;
		case 3:
			outputByServiceVert( pServer, &filter );
			break;
	}
	
	freeServers( pServer );

	if (pBuf != NULL)
      NetApiBufferFree(pBuf);


}
