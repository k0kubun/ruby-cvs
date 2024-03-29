#
# parser.rb
#
#   Copyright (c) 1999-2002 Minero Aoki <aamine@loveruby.net>
#
#   This program is free software.
#   You can distribute/modify this program under the same terms of ruby.
#
#   As a special exception, when this code is copied by Racc
#   into a Racc output file, you may use that output file
#   without restriction.
#
#   $Id$
#

unless defined? NotImplementedError then
  NotImplementedError = NotImplementError
end


module Racc
  class ParseError < StandardError; end
end
unless defined? ::ParseError then
  ParseError = Racc::ParseError
end


module Racc

  unless defined? Racc_No_Extentions then
    Racc_No_Extentions = false
  end

  class Parser

    Racc_Runtime_Version = '1.4.2'
    Racc_Runtime_Revision = '$Revision$'.split(/\s+/)[1]

    Racc_Runtime_Core_Version_R = '1.4.2'
    Racc_Runtime_Core_Revision_R = '$Revision$'.split(/\s+/)[1]
    begin
      require 'racc/cparse'
    # Racc_Runtime_Core_Version_C  = (defined in extention)
      Racc_Runtime_Core_Revision_C = Racc_Runtime_Core_Id_C.split(/\s+/)[2]
      unless new.respond_to? :_racc_do_parse_c, true then
        raise LoadError, 'old cparse.so'
      end
      if Racc_No_Extentions then
        raise LoadError, 'selecting ruby version of racc runtime core'
      end

      Racc_Main_Parsing_Routine    = :_racc_do_parse_c
      Racc_YY_Parse_Method         = :_racc_yyparse_c
      Racc_Runtime_Core_Version    = Racc_Runtime_Core_Version_C
      Racc_Runtime_Core_Revision   = Racc_Runtime_Core_Revision_C
      Racc_Runtime_Type            = 'c'
    rescue LoadError
      Racc_Main_Parsing_Routine    = :_racc_do_parse_rb
      Racc_YY_Parse_Method         = :_racc_yyparse_rb
      Racc_Runtime_Core_Version    = Racc_Runtime_Core_Version_R
      Racc_Runtime_Core_Revision   = Racc_Runtime_Core_Revision_R
      Racc_Runtime_Type            = 'ruby'
    end

    def self.racc_runtime_type
      Racc_Runtime_Type
    end


    private


    def _racc_setup
      t = self.class

      unless t::Racc_debug_parser then
        @yydebug = false
      end
      @yydebug = false unless defined? @yydebug

      if @yydebug then
        @racc_debug_out = $stderr unless defined? @racc_debug_out
        @racc_debug_out ||= $stderr
      end

      arg = t::Racc_arg
      if arg.size < 14 then
        arg[13] = true
      end
      arg
    end

    def _racc_init_sysvars
      @racc_state = [ 0 ]
      @racc_tstack = []
      @racc_vstack = []

      @racc_t = nil
      @racc_val = nil

      @racc_read_next = true

      @racc_user_yyerror = false
      @racc_error_status = 0
    end


    ###
    ### do_parse
    ###

    def do_parse
      __send__ Racc_Main_Parsing_Routine, _racc_setup(), false
    end

    def next_token
      raise NotImplementedError, "#{self.class}\#next_token is not defined"
    end

    def _racc_do_parse_rb( arg, in_debug )
      action_table, action_check, action_default, action_pointer,
      goto_table,   goto_check,   goto_default,   goto_pointer,
      nt_base,      reduce_table, token_table,    shift_n,
      reduce_n,     use_result,   * = arg

      _racc_init_sysvars
      tok = act = i = nil
      nerr = 0


      catch( :racc_end_parse ) {
      while true do

        if i = action_pointer[ @racc_state[-1] ] then
          if @racc_read_next then
            if @racc_t != 0 then   # not EOF
              tok, @racc_val = next_token()
              unless tok then   # EOF
                @racc_t = 0
              else
                @racc_t = (token_table[tok] or 1)   # error token
              end
              racc_read_token( @racc_t, tok, @racc_val ) if @yydebug

              @racc_read_next = false
            end
          end
          i += @racc_t
          if i >= 0 and act = action_table[i] and
             action_check[i] == @racc_state[-1] then
            ;
          else
            act = action_default[ @racc_state[-1] ]
          end
        else
          act = action_default[ @racc_state[-1] ]
        end

        while act = _racc_evalact( act, arg ) do end

      end
      }
    end


    ###
    ### yyparse
    ###

    def yyparse( recv, mid )
      __send__ Racc_YY_Parse_Method, recv, mid, _racc_setup(), true
    end

    def _racc_yyparse_rb( recv, mid, arg, c_debug )
      action_table, action_check, action_default, action_pointer,
      goto_table,   goto_check,   goto_default,   goto_pointer,
      nt_base,      reduce_table, token_table,    shift_n,
      reduce_n,     use_result,   * = arg

      _racc_init_sysvars
      tok = nil
      act = nil
      i = nil
      nerr = 0


      catch( :racc_end_parse ) {
        until i = action_pointer[ @racc_state[-1] ] do
          while act = _racc_evalact(
                  action_default[ @racc_state[-1] ], arg ) do end
        end

        recv.__send__( mid ) do |tok, val|
# $stderr.puts "rd: tok=#{tok}, val=#{val}"
          unless tok then
            @racc_t = 0
          else
            @racc_t = (token_table[tok] or 1)   # error token
          end
          @racc_val = val
          @racc_read_next = false

          i += @racc_t
          if i >= 0 and act = action_table[i] and
             action_check[i] == @racc_state[-1] then
# $stderr.puts "01: act=#{act}"
          else
            act = action_default[ @racc_state[-1] ]
# $stderr.puts "02: act=#{act}"
# $stderr.puts "curstate=#{@racc_state[-1]}"
          end

          while act = _racc_evalact( act, arg ) do end

          while not (i = action_pointer[ @racc_state[-1] ]) or
                not @racc_read_next or
                @racc_t == 0 do   # $
            if i and i += @racc_t and
               i >= 0 and
               act = action_table[i] and
               action_check[i] == @racc_state[-1] then
# $stderr.puts "03: act=#{act}"
              ;
            else
# $stderr.puts "04: act=#{act}"
              act = action_default[ @racc_state[-1] ]
            end

            while act = _racc_evalact( act, arg ) do end
          end
        end
      }
    end


    ###
    ### common
    ###

    def _racc_evalact( act, arg )
# $stderr.puts "ea: act=#{act}"
      action_table, action_check, action_default, action_pointer,
      goto_table,   goto_check,   goto_default,   goto_pointer,
      nt_base,      reduce_table, token_table,    shift_n,
      reduce_n,     use_result,   * = arg
nerr = 0   # tmp

      if act > 0 and act < shift_n then
        #
        # shift
        #

        if @racc_error_status > 0 then
          @racc_error_status -= 1 unless @racc_t == 1   # error token
        end

        @racc_vstack.push @racc_val
        @racc_state.push act
        @racc_read_next = true

        if @yydebug then
          @racc_tstack.push @racc_t
          racc_shift( @racc_t, @racc_tstack, @racc_vstack )
        end

      elsif act < 0 and act > -reduce_n then
        #
        # reduce
        #

        code = catch( :racc_jump ) {
          @racc_state.push _racc_do_reduce( arg, act )
          false
        }
        if code then
          case code
          when 1 # yyerror
            @racc_user_yyerror = true   # user_yyerror
            return -reduce_n
          when 2 # yyaccept
            return shift_n
          else
            raise RuntimeError, '[Racc Bug] unknown jump code'
          end
        end

      elsif act == shift_n then
        #
        # accept
        #

        racc_accept if @yydebug
        throw :racc_end_parse, @racc_vstack[0]

      elsif act == -reduce_n then
        #
        # error
        #

        case @racc_error_status
        when 0
          unless arg[21] then   # user_yyerror
            nerr += 1
            on_error @racc_t, @racc_val, @racc_vstack
          end
        when 3
          if @racc_t == 0 then   # is $
            throw :racc_end_parse, nil
          end
          @racc_read_next = true
        end
        @racc_user_yyerror = false
        @racc_error_status = 3

        while true do
          if i = action_pointer[ @racc_state[-1] ] then
            i += 1   # error token
            if i >= 0 and
               (act = action_table[i]) and
               action_check[i] == @racc_state[-1]  then
               break
            end
          end

          throw :racc_end_parse, nil if @racc_state.size < 2
          @racc_state.pop
          @racc_vstack.pop
          if @yydebug then
            @racc_tstack.pop
            racc_e_pop( @racc_state, @racc_tstack, @racc_vstack )
          end
        end

        return act

      else
        raise RuntimeError, "[Racc Bug] unknown action #{act.inspect}"
      end

      racc_next_state( @racc_state[-1], @racc_state ) if @yydebug

      nil
    end

    def _racc_do_reduce( arg, act )
      action_table, action_check, action_default, action_pointer,
      goto_table,   goto_check,   goto_default,   goto_pointer,
      nt_base,      reduce_table, token_table,    shift_n,
      reduce_n,     use_result,   * = arg
      state = @racc_state
      vstack = @racc_vstack
      tstack = @racc_tstack

      i = act * -3
      len       = reduce_table[i]
      reduce_to = reduce_table[i+1]
      method_id = reduce_table[i+2]
      void_array = []

      tmp_t = tstack[ -len, len ] if @yydebug
      tmp_v = vstack[ -len, len ]
      tstack[ -len, len ] = void_array if @yydebug
      vstack[ -len, len ] = void_array
      state[ -len, len ]  = void_array

      # tstack must be updated AFTER method call
      if use_result then
        vstack.push __send__(method_id, tmp_v, vstack, tmp_v[0])
      else
        vstack.push __send__(method_id, tmp_v, vstack)
      end
      tstack.push reduce_to

      racc_reduce( tmp_t, reduce_to, tstack, vstack ) if @yydebug

      k1 = reduce_to - nt_base
      if i = goto_pointer[ k1 ] then
        i += state[-1]
        if i >= 0 and (curstate = goto_table[i]) and goto_check[i] == k1 then
          return curstate
        end
      end
      goto_default[ k1 ]
    end

    def on_error( t, val, vstack )
      raise ParseError, sprintf("\nparse error on value %s (%s)",
                                val.inspect,
                                token_to_str(t) || '?')
    end

    def yyerror
      throw :racc_jump, 1
    end

    def yyaccept
      throw :racc_jump, 2
    end

    def yyerrok
      @racc_error_status = 0
    end


    # for debugging output

    def racc_read_token( t, tok, val )
      @racc_debug_out.print 'read    '
      @racc_debug_out.print tok.inspect, '(', racc_token2str(t), ') '
      @racc_debug_out.puts val.inspect
      @racc_debug_out.puts
    end

    def racc_shift( tok, tstack, vstack )
      @racc_debug_out.puts "shift   #{racc_token2str tok}"
      racc_print_stacks tstack, vstack
      @racc_debug_out.puts
    end

    def racc_reduce( toks, sim, tstack, vstack )
      out = @racc_debug_out
      out.print 'reduce '
      if toks.empty? then
        out.print ' <none>'
      else
        toks.each {|t| out.print ' ', racc_token2str(t) }
      end
      out.puts " --> #{racc_token2str(sim)}"
          
      racc_print_stacks tstack, vstack
      @racc_debug_out.puts
    end

    def racc_accept
      @racc_debug_out.puts 'accept'
      @racc_debug_out.puts
    end

    def racc_e_pop( state, tstack, vstack )
      @racc_debug_out.puts 'error recovering mode: pop token'
      racc_print_states state
      racc_print_stacks tstack, vstack
      @racc_debug_out.puts
    end

    def racc_next_state( curstate, state )
      @racc_debug_out.puts  "goto    #{curstate}"
      racc_print_states state
      @racc_debug_out.puts
    end

    def racc_print_stacks( t, v )
      out = @racc_debug_out
      out.print '        ['
      t.each_index do |i|
        out.print ' (', racc_token2str(t[i]), ' ', v[i].inspect, ')'
      end
      out.puts ' ]'
    end

    def racc_print_states( s )
      out = @racc_debug_out
      out.print '        ['
      s.each {|st| out.print ' ', st }
      out.puts ' ]'
    end

    def racc_token2str( tok )
      self.class::Racc_token_to_s_table[tok] or
        raise RuntimeError, "[Racc Bug] can't convert token #{tok} to string"
    end

    def token_to_str( t )
      self.class::Racc_token_to_s_table[t]
    end

  end

end
