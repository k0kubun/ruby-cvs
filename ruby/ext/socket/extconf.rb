require 'mkmf'
$LDFLAGS = "-L/usr/local/lib" if File.directory?("/usr/local/lib")
case PLATFORM
when /mswin32/
  test_func = "WSACleanup"
  have_library("wsock32", "WSACleanup")
when /cygwin32/
  test_func = "socket"
when /beos/
  test_func = "socket"
  have_library("net", "socket")
else
  test_func = "socket"
  have_library("nsl", "t_open")
  have_library("socket", "socket")
end
have_header("sys/un.h")
if have_func(test_func)
  have_func("inet_aton")
  have_func("hsterror")
  unless have_func("gethostname")
    have_func("uname")
  end
  if ENV["SOCKS_SERVER"]  # test if SOCKSsocket needed
    if have_library("socks", "Rconnect")
      $CFLAGS="-DSOCKS"
    end
  end
  create_makefile("socket")
end
