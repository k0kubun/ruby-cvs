require 'net/http'

data = $stdin.read

h = Net::HTTP.new("rubygarden.org")
resp, = h.post("/cgi-bin/rrr.rb", data)

if resp.code == "200"
  puts "Results uploaded..."
else
  puts "Failed to upload results:"
  puts "#{resp.code}: #{resp.message}"
end
