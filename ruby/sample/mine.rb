class Board
  def clr
    print "\e[2J"
  end
  def pos(x,y)
    printf "\e[%d;%dH", y+1, x*2+1
  end
  def colorstr(id,s)
    printf "\e[%dm%s\e[0m", id, s
  end
  def put(x, y, col, str)
    pos(x,y); colorstr(43,str)
    pos(0,@hi); print "$B;D$j(B:",@mc,"/",@total,"   "
    pos(x,y)
  end
  private :clr, :pos, :colorstr, :put
  CHR=["$B!&(B","$B#1(B","$B#2(B","$B#3(B","$B#4(B","$B#5(B","$B#6(B","$B#7(B","$B#8(B","$B!z(B","$B!|(B","@@"]
  COL=[46,43,45] # default,opened,over
  def initialize(h,w,m)
    # $B%2!<%`HW$N@8@.(B(h:$B=D!$(Bw:$B2#!$(Bm:$BGzCF$N?t(B)
    @hi=h; @wi=w; @m=m
    reset
  end
  def reset
    # $B%2!<%`HW$r(B($B:F(B)$B=i4|2=$9$k(B
    srand()
    @cx=0; @cy=0; @mc=@m
    @over=false
    @data=Array.new(@hi*@wi)
    @state=Array.new(@hi*@wi)
    @total=@hi*@wi
    @total.times {|i| @data[i]=0}
    @m.times do
       loop do
         j=rand(@total-1)
         if @data[j] == 0 then
           @data[j]=1
           break
         end
       end
    end
    clr; pos(0,0)
    @hi.times{|y| pos(0,y); colorstr(COL[0],CHR[0]*@wi)}
    pos(@cx,@cy)
  end
  def mark
    # $B8=:_$N%+!<%=%k0LCV$K%^!<%/$r$D$1$k(B
    if @state[@wi*@cy+@cx] != nil then return end
    @state[@wi*@cy+@cx] = "MARK"
    @mc=@mc-1;
    @total=@total-1;
    put(@cx, @cy, COL[1], CHR[9])
  end
  def open(x=@cx,y=@cy)
    # $B8=:_$N%+!<%=%k0LCV$r%*!<%W%s$K$9$k(B
    # $BGzCF$,$"$l$P%2!<%`%*!<%P!<(B
    if @state[@wi*y+x] =="OPEN"  then return 0 end
    if @state[@wi*y+x] == nil then @total=@total-1 end
    if @state[@wi*y+x] =="MARK" then @mc=@mc+1 end
    @state[@wi*y+x]="OPEN"
    if fetch(x,y) == 1 then @over = 1; return end
    c = count(x,y)
    put(x, y, COL[1], CHR[c])
    return 0 if c != 0
    if x > 0 && y > 0         then open(x-1,y-1) end
    if y > 0                  then open(x,  y-1) end
    if x < @wi-1 && y > 0     then open(x+1,y-1) end
    if x > 0                  then open(x-1,y) end
    if x < @wi-1              then open(x+1,y) end
    if x > 0 && y < @hi-1     then open(x-1,y+1) end
    if y < @hi -1             then open(x,y+1) end
    if x < @wi-1 && y < @hi-1 then open(x+1,y+1) end
    pos(@cx,@cy)
  end
  def fetch(x,y)
    # (x,y)$B$N0LCV$NGzCF$N?t(B(0 or 1)$B$rJV$9(B
    if x < 0 then 0
    elsif x >= @wi then 0
    elsif y < 0 then 0
    elsif y >= @hi then 0
    else
      @data[x*@wi+y]
    end
  end
  def count(x,y)
    # (x,y)$B$KNY@\$9$kGzCF$N?t$rJV$9(B
    fetch(x-1,y-1)+fetch(x,y-1)+fetch(x+1,y-1)+
    fetch(x-1,y)  +             fetch(x+1,y)+
    fetch(x-1,y+1)+fetch(x,y+1)+fetch(x+1,y+1)
  end
  def over(win)
    # $B%2!<%`$N=*N;(B
    quit
    unless win
      pos(@cx,@cy); print CHR[11]
    end
    pos(0,@hi)
    if win then print "*** YOU WIN !! ***"
    else print "*** GAME OVER ***"
    end
  end
  def over?
    # $B%2!<%`$N=*N;%A%'%C%/(B
    # $B=*N;=hM}$b8F$S=P$9(B
    remain = (@mc+@total == 0)
    if @over || remain
      over(remain)
      true
    else
      false
    end
  end
  def quit
    # $B%2!<%`$NCfCG(B($B$^$?$O=*N;(B)
    # $BHWLL$rA4$F8+$;$k(B
    @hi.times do|y|
      pos(0,y)
      @wi.times do|x|
	colorstr(if @state[y*@wi+x] == "MARK" then COL[1] else COL[2] end,
		 if fetch(x,y)==1 then CHR[10] else CHR[count(x,y)] end)
      end
    end
  end
  def down
    # $B%+!<%=%k$r2<$K(B
    if @cy < @hi-1 then @cy=@cy+1; pos(@cx, @cy) end
  end
  def up
    # $B%+!<%=%k$r>e$K(B
    if @cy > 0 then @cy=@cy-1; pos(@cx, @cy) end
  end
  def left
    # $B%+!<%=%k$r:8$K(B
    if @cx > 0 then @cx=@cx-1; pos(@cx, @cy) end
  end
  def right
    # $B%+!<%=%k$r1&$K(B
    if @cx < @wi-1 then @cx=@cx+1; pos(@cx, @cy) end
  end
end

bd=Board.new(10,10,10)
system("stty raw -echo")
begin
  loop do
    case getc
    when ?n  # new game
      bd.reset
    when ?m  # mark
      bd.mark
    when ?j
      bd.down
    when ?k
      bd.up
    when ?h
      bd.left
    when ?l
      bd.right
    when ?\s
      bd.open
    when ?q,?\C-c  # quit game
      bd.quit
      break
    end
    if bd.over?
      if getc == ?q then break end
      bd.reset
    end
  end
ensure
 system("stty -raw echo")
end
print "\n"
