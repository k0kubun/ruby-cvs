require 'net/http'

h = Net::HTTP.new("rubygarden.org")

data = $stdin.read

resp, = h.post("/cgi-bin/rrr.rb", data)

if resp.code == "200"
  puts "Results uploaded..."
else
  puts "Failed to upload results:"
  puts "#{resp.code}: #{resp.message}"
end
