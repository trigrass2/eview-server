#ifndef _SIG_PIPE_HANDLER_H_
#define _SIG_PIPE_HANDLER_H_

#include <ace/OS_NS_unistd.h>
#include <ace/Signal.h>
#include "pklog/pklog.h"
extern CPKLog PKLog;

class CSigPipeHandler : public ACE_Event_Handler
{
public:
	CSigPipeHandler () : signum_(SIGPIPE)
	{ }
	
	virtual ~CSigPipeHandler ()
	{ }
	
	virtual int handle_signal (int signum, siginfo_t * = 0, ucontext_t * = 0)
	{
		
		LH_LOG ((PK_LOGLEVEL_DEBUG, ACE_TEXT ("Pipe Broken Signal occured\n")));		
		return 0;
	}
	
private:
	int signum_;
};

#endif// _SIG_PIPE_HANDLER_H_
