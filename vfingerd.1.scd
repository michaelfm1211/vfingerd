VFINGERD(1)

# NAME

vfingerd - a virtual finger server

# SYNOPYSIS

*vfingerd* [*-h*] [*-a* addr] [*-p* port] [*-c* config]

# DESCRIPTION

*vfingerd* lets you set up a finger server entirely with a single config file. 
Every user known to *vfingerd* must have an entry in your config file, which is
/etc/vfingerd.conf by default. Users may be real login users with information
collected from GECOS or virtual users with all synthetic information given in
the config file. See the *CONFIGURATION* section below for help setting up
*vfingerd*.

# OPTIONS

*-h*
	Print the usage of the program.

*-a* _addr_
	Start the server to listen on address _addr_. The default is to listen
	on all addresses, or 0.0.0.0.

*-p* _port_
	Start the server to listen on port _port_. The default is to listen on
	port 79, which may require root privileges.

*-c* _config_
	Read and load the users listed in config file _config_. The default is
	to use /etc/vfingerd.conf. During installation, a default config file
	may be copied to /etc/vfingerd.conf (see the README for more details).
	This file may require editing to work on your machine.

# CONFIGURATION

*vfingerd* uses custom syntax for its config files, although it is intentionally
very similar to standard .ini files. Clear examples are provided in the default
config file. Sections are independent of each other, and each section configures
a single user. The section name is the user's finger username. The following is
a list of all possible keys that may be defined for a user:

*use_passwd* = boolean (default: _false_, not required)
	If _true_, then vfingerd will read information from the user's GECOS
	field. In order for to use this key, the user must be a set up as a real
	user on the system. Generally, if this is set to true, then no further
	configuration for the user is needed.

*hidden* = boolean (default: _false_, not required)
	If _true_, then vfingerd will not list the user in the list of all
	users. However, if the user is explicitly queried, then nothing special
	is done.

*alias* = string (default: none, not required)
	If a value is specified, the user will become an alias of another user
	specified by the provided string. Alias resolution is done as requests
	are handled and are not cached, so large numbers of aliases is not
	recommended. Generally, if a value is specified, then no further
	configuration is needed. One notable exception is that if a user is
	aliased to hidden user, both users must have *hidden* set to _true_ for
	both users to be hidden.

*name* = string (default: none, mostly required)
	The specified value is the real name of the user. Unless you are using
	*use_passwd* or *hidden* you must specify a value.

*plan* = string (default: none, not required)
	Specifies the user's plan as a string literal. If you wish to include
	line breaks within this key's value, then you must use heredoc syntax.
	An example is given in the default configuration file.

*plan\_file* = string (default: none, not required)
	Specifies the path to the user's plan file. The contents of the file is
	read and cached in memory at server startup, so if the server must be
	restarted if the contents of a user's plan file change.

# STANDARDS

*vfingerd* follows RFC742, but does not aim to be completely compliant with the
stricter RFC1288. *vfingerd* was built in the 21st century respects users'
privacy by only sending real names and usernames, but more information may be
manually added in a user's plan.

# AUTHORS

*vfingerd* was written by Michael M (https://github.com/michaelfm1211).

