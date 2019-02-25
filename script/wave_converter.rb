require 'wavefile'
require 'optparse'

class WaveConverter
  include WaveFile

  def initialize
    @wave_data = []

    @target_sample_size = 512
    @target_format = Format.new(:mono, :pcm_16, 44100)
  end

  def add_wave_table_from_file(file_name)
    puts "'#{file_name}' reading..."
    parse(file_name)
  end

  def dump_bin(out_dir, file_name="wav_dump.bin")
    # little endian
    IO.binwrite("#{out_dir}/#{file_name}", @wave_data.flatten.pack('s<*'))
  end

  def dump_source(out_dir, file_name="wavetable")
    File.open("#{out_dir}/#{file_name}.h", 'w') do |header_file|
      header_file.puts <<~TEXT
        #ifndef WAVETABLE_H_
        #define WAVETABLE_H_
        
        #define WAVETABLE_SIZE #{@wave_data.size}
        #define WAVETABLE_SAMPLE_SIZE #{@target_sample_size}

        extern const int16_t wavetables[WAVETABLE_SIZE][WAVETABLE_SAMPLE_SIZE];

        #endif /* WAVETABLE_H_ */
      TEXT
    end
    File.open("#{out_dir}/#{file_name}.c", 'w') do |source_file|
      source_file.puts <<-TEXT
#include "#{file_name}.h"

const int16_t wavetables[WAVETABLE_SIZE][WAVETABLE_SAMPLE_SIZE] = {
      TEXT
      @wave_data.each do |data|
        source_file.print "  {"
        data.each_with_index do |byte, idx|
          if idx % 16 == 0
            source_file.print "\n    "
          else
            source_file.print " "
          end
          source_file.printf "%+#06x,", byte 
        end
        source_file.puts <<-TEXT
  },
        TEXT
      end
      source_file.puts <<-TEXT
};
      TEXT
    end
  end

  private

  def parse(file_name)

    bytes = []
    reader = Reader.new(file_name)
    reader.each_buffer do |buffer|
      new_buffer = buffer.convert(@target_format)
      bytes.concat(new_buffer.samples) 
    end

    sample_size = bytes.size

    rate = sample_size.to_f / @target_sample_size

    new_bytes = []
    @target_sample_size.times do |i|
      idx_f = i.to_f * rate
      i_start = idx_f.floor
      i_end = idx_f.ceil
      middle_rate = idx_f - i_start.to_f
      if i_start == i_end
        new_bytes[i] = bytes[i_start]
      else
        new_bytes[i] = (bytes[i_start] * (1.0 - middle_rate) + bytes[i_end] * (middle_rate)).to_i
      end
    end

    @wave_data << new_bytes
  end
end

if __FILE__ == $0
  opt = OptionParser.new
  params = {}
  opt.on('-f', '--format=VALUE') {|v| params[:format] = v }
  opt.parse!(ARGV)
  
  src_dir = ARGV[0]
  out_dir = ARGV[1]
  target_files = Dir["#{src_dir}/*"]

  c = WaveConverter.new
  target_files.sort!
  target_files.each do |file_name|
    c.add_wave_table_from_file(file_name)
  end

  case params[:format]
  when 'src', 'source'
    c.dump_source(out_dir)
  when 'bin', 'binary'
    c.dump_bin(out_dir)
  else
    c.dump_bin(out_dir)
  end
end
