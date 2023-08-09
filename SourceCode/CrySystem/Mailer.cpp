////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   Mailer.cpp
//  Version:     v1.00
//  Created:     5/12/2001 by Timur.
//  Compilers:   Visual C++ 6.0
//  Description: Send Mail.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Mailer.h"

#ifdef WIN32

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//#include <mapi.h>

bool CMailer::SendMessage(const char *subject, 
													const char *messageBody,
													const std::vector<const char*> &_recipients, 
													const std::vector<const char*> &_attachments, 
													bool bShowDialog)
{
#if 0
	// Preserve directory, (Can be changed if attachment specified)
	char dir[MAX_PATH];
	GetCurrentDirectory( sizeof(dir),dir );

	// Load MAPI dll
	HMODULE hMAPILib = LoadLibrary("MAPI32.DLL");
	LPMAPISENDMAIL lpfnMAPISendMail = (LPMAPISENDMAIL) GetProcAddress(hMAPILib, "MAPISendMail"); 

	int numRecipients  = (int)_recipients.size();

	// Handle Attachments
	MapiFileDesc* attachments = new MapiFileDesc[_attachments.size()];

	int i = 0;
	for(unsigned int k=0; k<_attachments.size();k++)
	{
		FILE *file = fopen(_attachments[k],"r");
		if (!file)
			continue;
		fclose(file);

		attachments[i].ulReserved   = 0;
		attachments[i].flFlags      = 0;
		attachments[i].nPosition    = (ULONG)-1;
		attachments[i].lpszPathName = (char*)(const char*)_attachments[k];
		attachments[i].lpszFileName = NULL;
		attachments[i].lpFileType   = NULL;
		i++;
	}
	int numAttachments = i;

	// Handle Recipients
	MapiRecipDesc* recipients = new MapiRecipDesc[numRecipients];

	std::vector<string> addresses;
	addresses.resize( numRecipients );
	for (i = 0; i < numRecipients; i++)
	{
		addresses[i] = string("SMTP:") + _recipients[i];
	}

	for(i=0; i<numRecipients; i++)
	{
		recipients[i].ulReserved   = 0;
		recipients[i].ulRecipClass = MAPI_TO;
		recipients[i].lpszName     = (char*)(const char*)_recipients[i];
		recipients[i].lpszAddress  = (char*)addresses[i].c_str();
		recipients[i].ulEIDSize    = 0;
		recipients[i].lpEntryID    = NULL;
	}

	/*
	// Carbon copy.
	recipients[i].ulReserved   = 0;
	recipients[i].ulRecipClass = MAPI_TO;
	recipients[i].lpszName     = "Timur Davidenko";
	recipients[i].lpszAddress  = "timur@crytek.com"
	recipients[i].ulEIDSize    = 0;
	recipients[i].lpEntryID    = NULL;
	*/

	// Create a message. Most members are set to NULL or 0 (the user may set them)
	/*
	MapiMessage message =  {0,						        // reserved, must be 0
											    (const char*)_subject,      // subject
													(const char*)_messageBody,  // message body
													NULL,           // NULL = interpersonal message
													NULL,           // no date; MAPISendMail ignores it
													NULL,           // no conversation ID
													0L,             // no flags, MAPISendMail ignores it
													NULL,           // no originator, this is ignored too (automatically filled in)
													numRecipients,  // number of recipients
													recipients,     // recipients array
													numAttachments, // number of attachments
													attachments};   // the attachment structure
													*/
	
	MapiMessage message;
	memset( &message,0,sizeof(message) );
	message.lpszSubject = (char*)(const char*)subject;
	message.lpszNoteText = (char*)(const char*)messageBody;
	message.lpszMessageType = NULL;
	
	message.nRecipCount = numRecipients;
	message.lpRecips = recipients;

	message.nFileCount = numAttachments;
	message.lpFiles = attachments;

 
	//Next, the client calls the MAPISendMail function and stores the return status so it can detect whether the call succeeded. You should use a more sophisticated error reporting mechanism than the C library function printf.

	FLAGS flags = bShowDialog ? MAPI_DIALOG : 0;
	flags |= MAPI_LOGON_UI;
	ULONG err = (*lpfnMAPISendMail) (0L,          // use implicit session.
												           0L,          // ulUIParam; 0 is always valid
											             &message,    // the message being sent
											             flags,       // if user allowed to edit the message
											             0L);         // reserved; must be 0
	delete [] attachments;
	delete [] recipients;

	FreeLibrary(hMAPILib);   // Free DLL module through handle

	// Restore previous directory.
	SetCurrentDirectory( dir );

	if (err != SUCCESS_SUCCESS )
		return false;
#endif
	return true;
}

#else //WIN32

bool CMailer::SendMessage(const char *subject, 
													const char *messageBody,
													const std::vector<const char*> &_recipients, 
													const std::vector<const char*> &_attachments, 
													bool bShowDialog)
{
	return true;
}

#endif //WIN32