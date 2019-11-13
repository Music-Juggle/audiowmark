#include <string>
#include <vector>
#include <sndfile.h>
#include <assert.h>

#include "wavdata.hh"

class AudioInputStream
{
  virtual std::vector<float> read_frames (size_t count) = 0;
};

class AudioOutputStream
{
};

class SFInputStream : public AudioInputStream
{
  SNDFILE    *m_sndfile = nullptr;
  std::string m_error_blurb;
  int         m_n_channels = 0;
  int         m_n_values = 0;
  int         m_sample_rate = 0;

  enum class State {
    NEW,
    OPEN,
    CLOSED
  };
  State       m_state = State::NEW;

public:
  ~SFInputStream();

  bool                open (const std::string& filename);
  std::vector<float>  read_frames (size_t count);
  void                close();

  int
  n_channels() const
  {
    return m_n_channels;
  }
  int sample_rate() const;
  size_t
  n_values() const
  {
    return m_n_values;
  }
  size_t
  n_frames() const
  {
    return m_n_values / m_n_channels;
  }
  const char *error_blurb() const
  {
    return m_error_blurb.c_str();
  }
};

class StdoutWavOutputStream : public AudioOutputStream
{
};

using std::string;
using std::vector;

SFInputStream::~SFInputStream()
{
  close();
}

bool
SFInputStream::open (const string& filename)
{
  assert (m_state == State::NEW);

  SF_INFO sfinfo = { 0, };

  m_sndfile = sf_open (filename.c_str(), SFM_READ, &sfinfo);

  int error = sf_error (m_sndfile);
  if (error)
    {
      m_error_blurb = sf_strerror (m_sndfile);
      if (m_sndfile)
        {
          m_sndfile = nullptr;
          sf_close (m_sndfile);
        }
      return false;
    }

  m_n_channels  = sfinfo.channels;
  m_n_values    = sfinfo.frames * sfinfo.channels;
  m_sample_rate = sfinfo.samplerate;
  m_state       = State::OPEN;
  return true;
}

int
SFInputStream::sample_rate() const
{
  return m_sample_rate;
}

vector<float>
SFInputStream::read_frames (size_t count)
{
  assert (m_state == State::OPEN);

  vector<int> isamples (count * m_n_channels);
  sf_count_t r_count = sf_readf_int (m_sndfile, &isamples[0], count);

  /* reading a wav file and saving it again with the libsndfile float API will
   * change some values due to normalization issues:
   *   http://www.mega-nerd.com/libsndfile/FAQ.html#Q010
   *
   * to avoid the problem, we use the int API and do the conversion beween int
   * and float manually - the important part is that the normalization factors
   * used during read and write are identical
   */
  vector<float> result (r_count * m_n_channels);;
  const double norm = 1.0 / 0x80000000LL;
  for (size_t i = 0; i < result.size(); i++)
    result[i] = isamples[i] * norm;

  return result;
}

void
SFInputStream::close()
{
  if (m_state == State::OPEN)
    {
      assert (m_sndfile);
      sf_close (m_sndfile);

      m_state = State::CLOSED;
    }
}

int
main (int argc, char **argv)
{
  SFInputStream in;

  std::string filename = (argc >= 2) ? argv[1] : "-";
  if (!in.open (filename.c_str()))
    {
      fprintf (stderr, "teststream: open failed: %s\n", in.error_blurb());
      return 1;
    }
  vector<float> samples, all_samples;
  do
    {
      samples = in.read_frames (1024);
      all_samples.insert (all_samples.end(), samples.begin(), samples.end());
    }
  while (samples.size());

  WavData wav_data (all_samples, in.n_channels(), in.sample_rate(), 16);
  wav_data.save ("out.wav");
}
