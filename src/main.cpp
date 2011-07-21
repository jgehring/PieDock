/*
 *   O         ,-
 *  � o    . -�  '     ,-
 *   �  .�        ` . �,�
 *     ( �   ))     . (
 *      `-;_    . -� `.`.
 *          `._'       �
 *
 * Copyright (c) 2007-2011 Markus Fisch <mf@markusfisch.de>
 *
 * Licensed under the MIT license:
 * http://www.opensource.org/licenses/mit-license.php
 */
#include "Application.h"

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <libgen.h>
#include <stdlib.h>
#include <iostream>

bool stop = false;

/**
 * Signal handler
 *
 * @param id - signal id
 */
void signalHandler( int id )
{
	switch( id )
	{
		case SIGCHLD:
			// more than one process may have been terminated
        	while( waitpid( -1, 0, WNOHANG | WUNTRACED ) > 0 );
        	break;
		case SIGHUP:
		case SIGINT:
		case SIGTERM:
			stop = true;
			break;
	}

	return;
}

/**
 * Process entry
 *
 * @param argc - number of arguments
 * @param argv - pointer to pointers of arguments
 */
int main( int argc, char **argv )
{
	try
	{
		PieDock::Settings settings;
		char *menuName = 0;
		bool background = true;

		// parse arguments
		{
			char *binary = basename( *argv );

			while( --argc )
				if( **++argv == '-' )
					switch( *((*argv)+1) )
					{
						default:
							std::cerr << "Skipping unknown flag '" <<
								*((*argv)+1) << "'" << std::endl;
							break;
						case '?':
						case 'h':
							std::cout <<
								binary << " [hvrm]" << std::endl <<
								"\t-h         this help" << std::endl <<
								"\t-v         show version" << std::endl <<
								"\t-r FILE    path and name of alternative " <<
									"configuration file" << std::endl <<
								"\t-m [MENU]  show already running " <<
									"instance" << std::endl <<
								"\t-f         stay in foreground" << std::endl;
							return 0;
						case 'v':
							std::cout <<
								binary << " 1.3.3" <<
								std::endl <<
								"Copyright (c) 2007-2011" <<
								std::endl <<
								"Markus Fisch <mf@markusfisch.de>" <<
								std::endl <<
								"Tatiana Azundris <hacks@azundris.com>" <<
								std::endl <<
								std::endl <<
								"Licensed under the MIT license:" <<
								std::endl <<
								"http://www.opensource.org/licenses/mit-license.php" <<
								std::endl;
							return 0;
						case 'r':
							if( !--argc )
								throw "missing FILE argument";
							settings.setConfigurationFile( *++argv );
							break;
						case 'f':
							background = false;
							break;
						case 'm':
							if( argc > 1 &&
								**(argv+1) != '-' )
							{
								--argc;
								menuName = *++argv;
							}
							break;
					}
				else
					std::cerr << "skipping unknown argument \"" <<
						*argv << "\"" << std::endl;

			if( settings.getConfigurationFile().empty() )
				settings.setConfigurationFileFromBinary( binary );
		}

		if (background)
		{
			switch( fork() )
			{
				default:
					// terminate parent process to detach from shell
					return 0;
				case 0:
					// pursue in child process
					break;
				case -1:
					throw "cannot fork";
			}
		}

		// always open display after fork
		PieDock::Application a( settings );

		// if another instance is already running, wake it
		if( a.remote( menuName ) )
			return 0;

		// obtain new process group
		setsid();

		signal( SIGCHLD, signalHandler );
		signal( SIGHUP, signalHandler );
		signal( SIGINT, signalHandler );
		signal( SIGTERM, signalHandler );

		return a.run( &stop );
	}
	catch( const char *e )
	{
		std::cerr << e << std::endl;

		return -1;
	}
}
