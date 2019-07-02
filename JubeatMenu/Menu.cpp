#include <stdio.h>
#include <windows.h>

#include "Menu.h"

Menu::Menu(_TCHAR *inifile)
{
	/* Read settings */
	settings = LoadSettings( inifile, &num_programs );

	/* For exiting on defaults */
	ftime(&beginning);
}

Menu::~Menu(void)
{
}

void Menu::ResetTimeout()
{
	ftime(&beginning);
}

void Menu::Tick()
{
	ftime(&current);
}

bool Menu::ShouldBootDefault()
{
	ftime(&current);
	return (current.time - beginning.time) > TIMEOUT_SECONDS;
}

unsigned int Menu::SecondsLeft()
{
	ftime(&current);
	int seconds = (int)(TIMEOUT_SECONDS - (current.time - beginning.time));
	return seconds >= 0 ? seconds : 0;
}

/**
 * Loads an INI file with the following format:
 *
 * [Name of game to launch]
 * launch=<location of batch/executable>
 */
launcher_program_t *Menu::LoadSettings( _TCHAR *ini_file, unsigned int *final_length )
{
	launcher_program_t *progs = 0;
	*final_length = 0;
	unsigned int got_name = 0;
	launcher_program_t temp;

	// Open the file
	HANDLE hFile = CreateFile(ini_file, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	char buffer[16384];
	unsigned int eof = 0;
	unsigned int eol = 0;
	unsigned int buflen = 0;

	if (hFile == 0)
	{
		return progs;
	}

	memset( &temp, 0, sizeof(temp) );
	memset( buffer, 0, sizeof(buffer) );

	while( !eof )
	{
		DWORD length;
		ReadFile( hFile, buffer + buflen, 1, &length, 0 );
		if (length == 0)
		{
			eof = 1;
			eol = 1;
		}
		else if( *(buffer + buflen) == '\r' )
		{
			/* Ignore \r completely */
			*(buffer + buflen) = 0;
		}
		else if( *(buffer + buflen) == '\n' )
		{
			/* End of line */
			*(buffer + buflen) = 0;
			eol = 1;
		}
		else
		{
			/* Valid thing */
			buflen++;
		}

		if ( eol == 1 )
		{
			/* Process line */
			if (buffer[0] == '[' && buffer[buflen - 1] == ']' && buflen > 2)
			{
				buffer[buflen - 1] = 0;
				char *game = buffer + 1;

				/* Copy this into temp structure */
				strcpy_s( temp.name, MAX_GAME_NAME_LENGTH, game );
				got_name = 1;
			}
			else
			{
				if (strncmp(buffer, "launch", 6) == 0) {
					unsigned int loc = 6;
					// Find equals sign after space
					while (loc < buflen && (buffer[loc] == ' ' || buffer[loc] == '\t')) { loc++; }
					if (loc < buflen)
					{
						if (buffer[loc] == '=')
						{
							loc++;
							while (loc < buflen && (buffer[loc] == ' ' || buffer[loc] == '\t')) { loc++; }
							if (loc < buflen)
							{
								char *launch = buffer + loc;

								if( got_name == 1 )
								{
									/* We have a name to associate with this */
									strcpy_s( temp.location, MAX_GAME_LOCATION_LENGTH, launch );
									got_name = 0;

									/* Make a new spot for this, copy in */
									(*final_length)++;
									progs = (launcher_program_t *)realloc( progs, sizeof(launcher_program_t) * (*final_length) );
									memcpy( progs + ((*final_length) - 1), &temp, sizeof(launcher_program_t) );
									memset( &temp, 0, sizeof(temp) );
								}
							}
						}
					}
				}
			}

			/* Reset buffer */
			if (buflen > 0)
			{
				memset( buffer, 0, sizeof(buffer) );
				buflen = 0;
			}

			/* Not end of line anymore */
			eol = 0;
		}
	}

	return progs;
}