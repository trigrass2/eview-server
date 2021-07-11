// $Id: reader.cpp 91671 2010-09-08 18:39:23Z johnnyw $

#include "ace/DEV_Addr.h"
#include "ace/DEV_Connector.h"
#include "ace/TTY_IO.h"



int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  if (argc < 2)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("usage: %s device-filename\n"),
                       argv[0]),
                      1);

  ACE_TTY_IO read_dev;
  {
	  ACE_DEV_Connector con;

	  if (con.connect (read_dev,
					   ACE_DEV_Addr (argv[1])) == -1)
		ACE_ERROR_RETURN ((LM_ERROR,
						   ACE_TEXT ("%p\n"),
						   argv[1]),
						  1);

	  ACE_TTY_IO::Serial_Params myparams;
	  myparams.baudrate = 9600;
	  myparams.xonlim = 0;
	  myparams.xofflim = 0;
	  myparams.readmincharacters = 0;
	  myparams.readtimeoutmsec = 10*1000; // 10 seconds
	  myparams.paritymode = "none";
	  myparams.ctsenb = false;
	  //myparams.rtsenb = 0; // should be 1
	  myparams.rtsenb = 1; // should be 1
	  myparams.xinenb = false;
	  myparams.xoutenb = false;
	  myparams.modem = false;
	  myparams.rcvenb = true;
	  myparams.dsrenb = false;
	  myparams.dtrdisable = false;
	  myparams.databits = 8;
	  myparams.stopbits = 1;

	  if (read_dev.control (ACE_TTY_IO::SETPARAMS,
							&myparams) == -1)
		ACE_ERROR_RETURN ((LM_ERROR,
						   ACE_TEXT ("%p control\n"),
						   argv[1]),
						  1);
  }

  // 7E 7E 7E 01 05 00 00 92 7A 33 9F
  char szHeartBeat[] = {0x7E,0x7E,0x7E,0x01,0x05,0x00,0x00,0x92,0x7A,0x33,0x9F};
  ssize_t bytes_written = read_dev.send_n ((void *) szHeartBeat, 11);

  //Sleep(1000);
  // Read till character 'q'.
  char szRecv[1024] = {0};
  ssize_t bytes_read = read_dev.recv ((void *) szRecv, 1);

  return 0;
}
