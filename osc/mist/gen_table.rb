TABLE_SIZE = 256

def gen_log_curve
  table = []
  top_value = Math.log(TABLE_SIZE)
  (1..TABLE_SIZE).each do |i|
    table << Math.log(i) / top_value
  end
  table
end

def print_table(table)
  puts "{"
    puts "  #{table.join(', ')}"
  puts "},"
end

print_table(gen_log_curve)
