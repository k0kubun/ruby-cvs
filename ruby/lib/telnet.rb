=begin
$Date$

== SIMPLE TELNET CLIANT LIBRARY

telnet.rb

Version 0.40

Wakou Aoyama <wakou@fsinet.or.jp>


=== MAKE NEW TELNET OBJECT

	host = Telnet.new({"Binmode" => false,           # default: false
	                   "Host" => "localhost",        # default: "localhost"
	                   "Output_log" => "output_log", # default: not output
	                   "Dump_log" => "dump_log",     # default: not output
	                   "Port" => 23,                 # default: 23
	                   "Prompt" => /[$%#>] \z/n,     # default: /[$%#>] \z/n
	                   "Telnetmode" => true,         # default: true
	                   "Timeout" => 10,              # default: 10
	                     # if ignore timeout then set "Timeout" to false.
	                   "Waittime" => 0,              # default: 0
	                   "Proxy" => proxy})            # default: nil
                                    # proxy is Telnet or TCPsocket object

Telnet object has socket class methods.

if set "Telnetmode" option to false. not telnet command interpretation.
"Waittime" is time to confirm "Prompt". There is a possibility that
the same character as "Prompt" is included in the data, and, when
the network or the host is very heavy, the value is enlarged.

=== STATUS OUTPUT

	host = Telnet.new({"Host" => "localhost"}){|c| print c }

connection status output.

example

	Trying localhost...
	Connected to localhost.


=== WAIT FOR MATCH

	line = host.waitfor(/match/)
	line = host.waitfor({"Match"   => /match/,
	                     "String"  => "string",
	                     "Timeout" => secs})
	                       # if ignore timeout then set "Timeout" to false.

if set "String" option, then Match == Regexp.new(quote("string"))


==== REALTIME OUTPUT

	host.waitfor(/match/){|c| print c }
	host.waitfor({"Match"   => /match/,
	              "String"  => "string",
	              "Timeout" => secs}){|c| print c}

of cource, set sync=true or flush is necessary.


=== SEND STRING AND WAIT PROMPT

	line = host.cmd("string")
	line = host.cmd({"String" => "string",
	                 "Prompt" => /[$%#>] \z/n,
	                 "Timeout" => 10})


==== REALTIME OUTPUT

	host.cmd("string"){|c| print c }
	host.cmd({"String" => "string",
	          "Prompt" => /[$%#>] \z/n,
	          "Timeout" => 10}){|c| print c }

of cource, set sync=true or flush is necessary.


=== SEND STRING

	host.print("string")


=== TURN TELNET COMMAND INTERPRETATION

	host.telnetmode        # turn on/off
	host.telnetmode(true)  # on
	host.telnetmode(false) # off


=== TOGGLE NEWLINE TRANSLATION

	host.binmode        # turn true/false
	host.binmode(true)  # no translate newline
	host.binmode(false) # translate newline


=== LOGIN

	host.login("username", "password")
	host.login({"Name" => "username",
	            "Password" => "password",
	            "Prompt" => /[$%#>] \z/n,
	            "Timeout" => 10})

if no password prompt.

	host.login("username")
	host.login({"Name" => "username",
	            "Prompt" => /[$%#>] \z/n,
	            "Timeout" => 10})


==== REALTIME OUTPUT

	host.login("username", "password"){|c| print c }
	host.login({"Name" => "username",
	            "Password" => "password",
	            "Prompt" => /[$%#>] \z/n,
	            "Timeout" => 10}){|c| print c }

of cource, set sync=true or flush is necessary.


== EXAMPLE

=== LOGIN AND SEND COMMAND

	localhost = Telnet.new({"Host" => "localhost",
	                        "Timeout" => 10,
	                        "Prompt" => /[$%#>] \z/n})
	localhost.login("username", "password"){|c| print c }
	localhost.cmd("command"){|c| print c }
	localhost.close


=== CHECKS A POP SERVER TO SEE IF YOU HAVE MAIL

	pop = Telnet.new({"Host" => "your_destination_host_here",
	                  "Port" => 110,
	                  "Telnetmode" => false,
	                  "Prompt" => /^\+OK/n})
	pop.cmd("user " + "your_username_here"){|c| print c}
	pop.cmd("pass " + "your_password_here"){|c| print c}
	pop.cmd("list"){|c| print c}


== HISTORY

=== Version 0.40

1999/09/17 17:41:41

- bug fix: preprocess method

=== Version 0.30

1999/09/14 23:09:05

- change prompt check order.
	not IO::select([@sock], nil, nil, waittime) and prompt === line
	--> prompt === line and not IO::select([@sock], nil, nil, waittime)

=== Version 0.24

1999/09/13 22:28:33

- Telnet#login
if ommit password, then not require password prompt.

=== Version 0.232

1999/08/10 05:20:21

- STATUS OUTPUT sample code typo. thanks to Tadayoshi Funaba <tadf@kt.rim.or.jp>
	host = Telnet.new({"Hosh" => "localhost"){|c| print c }
	--> host = Telnet.new({"Host" => "localhost"){|c| print c }

=== Version 0.231

1999/07/16 13:39:42

- TRUE --> true, FALSE --> false

=== Version 0.23

1999/07/15 22:32:09

- waitfor: if end of file reached, then return nil.

=== Version 0.22

1999/06/29 09:08:51

- new, waitfor, cmd: {"Timeout" => false}  # ignore timeout

=== Version 0.21

1999/06/28 18:18:55

- waitfor: not rescue (EOFError)

=== Version 0.20

1999/06/04 06:24:58

- waitfor: support for divided telnet command

=== Version 0.181

1999/05/22

- bug fix: print method

=== Version 0.18

1999/05/14

- respond to "IAC WON'T SGA" with "IAC DON'T SGA"
- DON'T SGA : end of line --> CR + LF
- bug fix: preprocess method

=== Version 0.17

1999/04/30

- bug fix: $! + "\n"  -->  $!.to_s + "\n"

=== Version 0.163

1999/04/11

- STDOUT.write(message) --> yield(message) if iterator?

=== Version 0.162

1999/03/17

- add "Proxy" option
- required timeout.rb

=== Version 0.161

1999/02/03

- select --> IO::select

=== Version 0.16

1998/10/09

- preprocess method change for the better
- add binmode method.
- change default Binmode. TRUE --> FALSE

=== Version 0.15

1998/10/04

- add telnetmode method.

=== Version 0.141

1998/09/22

- change default prompt. /[$%#>] $/ --> /[$%#>] \Z/

=== Version 0.14

1998/09/01

- IAC WILL SGA             send EOL --> CR+NULL
- IAC WILL SGA IAC DO BIN  send EOL --> CR
- NONE                     send EOL --> LF
- add Dump_log option.

=== Version 0.13

1998/08/25

- add print method.

=== Version 0.122

1998/08/05

- support for HP-UX 10.20    thanks to WATANABE Tetsuya <tetsu@jpn.hp.com>
- socket.<< --> socket.write

=== Version 0.121

1998/07/15

- string.+= --> string.concat

=== Version 0.12

1998/06/01

- add timeout, waittime.

=== Version 0.11

1998/04/21

- add realtime output.

=== Version 0.10

1998/04/13

- first release.

=end

require "socket"
require "delegate"
require "thread"
require "timeout"
TimeOut = TimeoutError

class Telnet < SimpleDelegator

  IAC   = 255.chr # "\377" # interpret as command:
  DONT  = 254.chr # "\376" # you are not to use option
  DO    = 253.chr # "\375" # please, you use option
  WONT  = 252.chr # "\374" # I won't use option
  WILL  = 251.chr # "\373" # I will use option
  SB    = 250.chr # "\372" # interpret as subnegotiation
  GA    = 249.chr # "\371" # you may reverse the line
  EL    = 248.chr # "\370" # erase the current line
  EC    = 247.chr # "\367" # erase the current character
  AYT   = 246.chr # "\366" # are you there
  AO    = 245.chr # "\365" # abort output--but let prog finish
  IP    = 244.chr # "\364" # interrupt process--permanently
  BREAK = 243.chr # "\363" # break
  DM    = 242.chr # "\362" # data mark--for connect. cleaning
  NOP   = 241.chr # "\361" # nop
  SE    = 240.chr # "\360" # end sub negotiation
  EOR   = 239.chr # "\357" # end of record (transparent mode)
  ABORT = 238.chr # "\356" # Abort process
  SUSP  = 237.chr # "\355" # Suspend process
  EOF   = 236.chr # "\354" # End of file
  SYNCH = 242.chr # "\362" # for telfunc calls

  OPT_BINARY         =   0.chr # "\000" # Binary Transmission
  OPT_ECHO           =   1.chr # "\001" # Echo
  OPT_RCP            =   2.chr # "\002" # Reconnection
  OPT_SGA            =   3.chr # "\003" # Suppress Go Ahead
  OPT_NAMS           =   4.chr # "\004" # Approx Message Size Negotiation
  OPT_STATUS         =   5.chr # "\005" # Status
  OPT_TM             =   6.chr # "\006" # Timing Mark
  OPT_RCTE           =   7.chr # "\a"   # Remote Controlled Trans and Echo
  OPT_NAOL           =   8.chr # "\010" # Output Line Width
  OPT_NAOP           =   9.chr # "\t"   # Output Page Size
  OPT_NAOCRD         =  10.chr # "\n"   # Output Carriage-Return Disposition
  OPT_NAOHTS         =  11.chr # "\v"   # Output Horizontal Tab Stops
  OPT_NAOHTD         =  12.chr # "\f"   # Output Horizontal Tab Disposition
  OPT_NAOFFD         =  13.chr # "\r"   # Output Formfeed Disposition
  OPT_NAOVTS         =  14.chr # "\016" # Output Vertical Tabstops
  OPT_NAOVTD         =  15.chr # "\017" # Output Vertical Tab Disposition
  OPT_NAOLFD         =  16.chr # "\020" # Output Linefeed Disposition
  OPT_XASCII         =  17.chr # "\021" # Extended ASCII
  OPT_LOGOUT         =  18.chr # "\022" # Logout
  OPT_BM             =  19.chr # "\023" # Byte Macro
  OPT_DET            =  20.chr # "\024" # Data Entry Terminal
  OPT_SUPDUP         =  21.chr # "\025" # SUPDUP
  OPT_SUPDUPOUTPUT   =  22.chr # "\026" # SUPDUP Output
  OPT_SNDLOC         =  23.chr # "\027" # Send Location
  OPT_TTYPE          =  24.chr # "\030" # Terminal Type
  OPT_EOR            =  25.chr # "\031" # End of Record
  OPT_TUID           =  26.chr # "\032" # TACACS User Identification
  OPT_OUTMRK         =  27.chr # "\e"   # Output Marking
  OPT_TTYLOC         =  28.chr # "\034" # Terminal Location Number
  OPT_3270REGIME     =  29.chr # "\035" # Telnet 3270 Regime
  OPT_X3PAD          =  30.chr # "\036" # X.3 PAD
  OPT_NAWS           =  31.chr # "\037" # Negotiate About Window Size
  OPT_TSPEED         =  32.chr # " "    # Terminal Speed
  OPT_LFLOW          =  33.chr # "!"    # Remote Flow Control
  OPT_LINEMODE       =  34.chr # "\""   # Linemode
  OPT_XDISPLOC       =  35.chr # "#"    # X Display Location
  OPT_OLD_ENVIRON    =  36.chr # "$"    # Environment Option
  OPT_AUTHENTICATION =  37.chr # "%"    # Authentication Option
  OPT_ENCRYPT        =  38.chr # "&"    # Encryption Option
  OPT_NEW_ENVIRON    =  39.chr # "'"    # New Environment Option
  OPT_EXOPL          = 255.chr # "\377" # Extended-Options-List

  NULL = "\000"
  CR   = "\015"
  LF   = "\012"
  EOL  = CR + LF
v = $-v
$-v = false
  VERSION = "0.40"
  RELEASE_DATE = "$Date$"
$-v = v

  def initialize(options)
    @options = options
    @options["Binmode"]    = false         unless @options.key?("Binmode")
    @options["Host"]       = "localhost"   unless @options.key?("Host")
    @options["Port"]       = 23            unless @options.key?("Port")
    @options["Prompt"]     = /[$%#>] \z/n  unless @options.key?("Prompt")
    @options["Telnetmode"] = true          unless @options.key?("Telnetmode")
    @options["Timeout"]    = 10            unless @options.key?("Timeout")
    @options["Waittime"]   = 0             unless @options.key?("Waittime")

    @telnet_option = { "SGA" => false, "BINARY" => false }

    if @options.key?("Output_log")
      @log = File.open(@options["Output_log"], 'a+')
      @log.sync = true
      @log.binmode
    end

    if @options.key?("Dump_log")
      @dumplog = File.open(@options["Dump_log"], 'a+')
      @dumplog.sync = true
      @dumplog.binmode
    end

    if @options.key?("Proxy")
      if @options["Proxy"].kind_of?(Telnet)
        @sock = @options["Proxy"].sock
      elsif @options["Proxy"].kind_of?(TCPsocket)
        @sock = @options["Proxy"]
      else
        raise "Error; Proxy is Telnet or TCPSocket object."
      end
    else
      message = "Trying " + @options["Host"] + "...\n"
      yield(message) if iterator?
      @log.write(message) if @options.key?("Output_log")
      @dumplog.write(message) if @options.key?("Dump_log")

      begin
        if @options["Timeout"] == false
          @sock = TCPsocket.open(@options["Host"], @options["Port"])
        else
          timeout(@options["Timeout"]){
            @sock = TCPsocket.open(@options["Host"], @options["Port"])
          }
        end
      rescue TimeoutError
        raise TimeOut, "timed-out; opening of the host"
      rescue
        @log.write($!.to_s + "\n") if @options.key?("Output_log")
        @dumplog.write($!.to_s + "\n") if @options.key?("Dump_log")
        raise
      end
      @sock.sync = true
      @sock.binmode

      message = "Connected to " + @options["Host"] + ".\n"
      yield(message) if iterator?
      @log.write(message) if @options.key?("Output_log")
      @dumplog.write(message) if @options.key?("Dump_log")
    end

    super(@sock)
  end # initialize

  attr :sock

  def telnetmode(mode = 'turn')
    if 'turn' == mode
      @options["Telnetmode"] = @options["Telnetmode"] ? false : true
    else
      @options["Telnetmode"] = mode ? true : false
    end
  end

  def binmode(mode = 'turn')
    if 'turn' == mode
      @options["Binmode"] = @options["Binmode"] ? false : true
    else
      @options["Binmode"] = mode ? true : false
    end
  end

  def preprocess(string)
    str = string.dup

    # combine CR+NULL into CR
    str.gsub!(/#{CR}#{NULL}/no, CR) if @options["Telnetmode"]

    # combine EOL into "\n"
    str.gsub!(/#{EOL}/no, "\n") unless @options["Binmode"]

    str.gsub!(/#{IAC}(
                 #{IAC}|
                 #{AYT}|
                 [#{DO}#{DONT}#{WILL}#{WONT}]
                   [#{OPT_BINARY}-#{OPT_NEW_ENVIRON}#{OPT_EXOPL}]
               )/xno){
      if    IAC == $1         # handle escaped IAC characters
        IAC
      elsif AYT == $1         # respond to "IAC AYT" (are you there)
        @sock.write("nobody here but us pigeons" + EOL)
        ''
      elsif DO[0] == $1[0]    # respond to "IAC DO x"
        if OPT_BINARY[0] == $1[1]
          @telnet_option["BINARY"] = true
          @sock.write(IAC + WILL + OPT_BINARY)
        else
          @sock.write(IAC + WONT + $1[1..1])
        end
        ''
      elsif DONT[0] == $1[0]  # respond to "IAC DON'T x" with "IAC WON'T x"
        @sock.write(IAC + WONT + $1[1..1])
        ''
      elsif WILL[0] == $1[0]  # respond to "IAC WILL x"
        if    OPT_ECHO[0] == $1[1]
          @sock.write(IAC + DO + OPT_ECHO)
        elsif OPT_SGA[0]  == $1[1]
          @telnet_option["SGA"] = true
          @sock.write(IAC + DO + OPT_SGA)
        end
        ''
      elsif WONT[0] == $1[0]  # respond to "IAC WON'T x"
        if    OPT_ECHO[0] == $1[1]
          @sock.write(IAC + DONT + OPT_ECHO)
        elsif OPT_SGA[0]  == $1[1]
          @telnet_option["SGA"] = false
          @sock.write(IAC + DONT + OPT_SGA)
        end
        ''
      end
    }

    str
  end # preprocess

  def waitfor(options)
    time_out = @options["Timeout"]
    waittime = @options["Waittime"]

    if options.kind_of?(Hash)
      prompt   = if options.key?("Match")
                   options["Match"]   
                 elsif options.key?("Prompt")
                   options["Prompt"]
                 elsif options.key?("String")
                   Regexp.new( Regexp.quote(options["String"]) )
                 end
      time_out = options["Timeout"]  if options.key?("Timeout")
      waittime = options["Waittime"] if options.key?("Waittime")
    else
      prompt = options
    end

    if time_out == false
      time_out = nil
    end

    line = ''
    buf = ''
    until(prompt === line and not IO::select([@sock], nil, nil, waittime))
      unless IO::select([@sock], nil, nil, time_out)
        raise TimeOut, "timed-out; wait for the next data"
      end
      begin
        c = @sock.sysread(1024 * 1024)
        @dumplog.print(c) if @options.key?("Dump_log")
        buf.concat c
        if @options["Telnetmode"]
          buf = preprocess(buf) 
          if /#{IAC}.?\z/no === buf
            next
          end 
        end 
        @log.print(buf) if @options.key?("Output_log")
        yield buf if iterator?
        line.concat(buf)
        buf = ''
      rescue EOFError # End of file reached
        if line == ''
          line = nil
          yield nil if iterator?
        end
        break
      end
    end
    line
  end

  def print(string)
    str = string.dup + "\n"

    str.gsub!(/#{IAC}/no, IAC + IAC) if @options["Telnetmode"]

    unless @options["Binmode"]
      if @telnet_option["BINARY"] and @telnet_option["SGA"]
        # IAC WILL SGA IAC DO BIN send EOL --> CR
        str.gsub!(/\n/n, CR)
      elsif @telnet_option["SGA"]
        # IAC WILL SGA send EOL --> CR+NULL
        str.gsub!(/\n/n, CR + NULL)
      else
        # NONE send EOL --> CR+LF
        str.gsub!(/\n/n, EOL)
      end
    end

    @sock.write(str)
  end

  def cmd(options)
    match    = @options["Prompt"]
    time_out = @options["Timeout"]

    if options.kind_of?(Hash)
      string   = options["String"]
      match    = options["Match"]   if options.key?("Match")
      time_out = options["Timeout"] if options.key?("Timeout")
    else
      string = options
    end

    IO::select(nil, [@sock])
    self.print(string)
    if iterator?
      waitfor({"Prompt" => match, "Timeout" => time_out}){|c| yield c }
    else
      waitfor({"Prompt" => match, "Timeout" => time_out})
    end
  end

  def login(options, password = nil)
    if options.kind_of?(Hash)
      username = options["Name"]
      password = options["Password"]
    else
      username = options
    end

    if iterator?
      line = waitfor(/login[: ]*\z/n){|c| yield c }
      if password
        line.concat( cmd({"String" => username,
                          "Match" => /Password[: ]*\z/n}){|c| yield c } )
        line.concat( cmd(password){|c| yield c } )
      else
        line.concat( cmd(username){|c| yield c } )
      end
    else
      line = waitfor(/login[: ]*\z/n)
      if password
        line.concat( cmd({"String" => username,
                          "Match" => /Password[: ]*\z/n}) )
        line.concat( cmd(password) )
      else
        line.concat( cmd(username) )
      end
    end
    line
  end

end
