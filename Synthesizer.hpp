#ifndef SYNTHESIZER_HPP
#define SYNTHESIZER_HPP

#pragma region license
/*
	BSD 3-Clause License

	Copyright (c) 2023, Alex

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its
	   contributors may be used to endorse or promote products derived from
	   this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma endregion

#include <cstdint>
#include <cmath>
#include <vector>
#include <list>
#include <mutex>

#ifndef fwave_t
	#define fwave_t double
#endif

#ifndef string_t
	#include <string>
	#ifdef _UNICODE
		#define string_t std::wstring
		#define _T(x) L##x
	#else
		#define string_t std::string
		#define _T(x) x
	#endif
#endif

#ifdef SYNTH_APPROX
	fwave_t approxsin(fwave_t x)
	{
		x *= 0.15915;
		x -= (int32_t)x;
		return 20.875 * x * (x - 0.5) * (x - 1.0);
	}

	// TODO: Approx asin

	#define wave(x) approxsin(x)
#else
	#define wave(x) sin(x)
#endif

namespace synth
{
	constexpr fwave_t PI = 3.14159;
	const fwave_t TWLTH_ROOT_OF_2 = pow(2.0, 1.0 / 12.0);

	enum class oscillator : uint8_t
	{
		SINE,
		TRIANGLE,
		SQUARE,
		SAW_ANA,
		SAW_DIG,
		NOISE
	};

	fwave_t ang(fwave_t freq);
	fwave_t osc(fwave_t time, fwave_t hertz, oscillator oscillator, fwave_t lfo_hertz = 0.0, fwave_t lfo_amplitude = 0.0, fwave_t saw_period = 50.0);

	namespace inst { struct base; }

	struct note
	{
		int32_t id;
		fwave_t on, off;
		bool active;

		inst::base* channel;

		note();
	};

	fwave_t scale(fwave_t freq, int32_t noteID);

	namespace envelope
	{
		struct base
		{
			virtual fwave_t get_amplitude(fwave_t time_global, fwave_t time_on, fwave_t time_off) = 0;
		};

		struct adsr : base
		{
			fwave_t attackTime;
			fwave_t decayTime;
			fwave_t sustainAmplitude;
			fwave_t releaseTime;
			fwave_t startAmplitude;

			adsr();
			fwave_t get_amplitude(fwave_t time_global, fwave_t time_on, fwave_t time_off) override;
		};
	}

	namespace filter
	{
		struct base
		{
			fwave_t alpha;
			fwave_t prevSample;
			fwave_t hertz;

			base(fwave_t freq = 120.0, fwave_t hertz = 44100.0);

			void set_frequency(fwave_t freq);
			virtual fwave_t filter(fwave_t time_global, fwave_t sample) = 0;
		};

		struct low_pass : base
		{
			low_pass(fwave_t freq = 120.0, fwave_t hertz = 44100.0);
			virtual fwave_t filter(fwave_t time_global, fwave_t sample) override;
		};

		struct high_pass : base
		{
			high_pass(fwave_t freq = 120.0, fwave_t hertz = 44100.0);
			virtual fwave_t filter(fwave_t time_global, fwave_t sample) override;
		};
	}

	namespace inst
	{
		struct base
		{
			fwave_t volume;
			fwave_t maxLifeTime;
			string_t name;

			envelope::adsr env;

			virtual fwave_t sound(fwave_t time_global, const note& note, bool& note_finished) = 0;

			bool is_note_finished(fwave_t amplitude, bool checkAmplitude = true, fwave_t timeGlobal = 0.0, fwave_t timeOn = 0.0);
		};

		struct bell : base
		{
			bell();
			virtual fwave_t sound(fwave_t time_global, const note& note, bool& note_finished) override;
		};

		struct harmonica : base
		{
			harmonica();
			fwave_t sound(fwave_t time_global, const note& note, bool& note_finished) override;
		};

		namespace drum
		{
			struct kick : base
			{
				kick();
				fwave_t sound(fwave_t time_global, const note& note, bool& note_finished) override;
			};

			struct snare : base
			{
				snare();
				fwave_t sound(fwave_t time_global, const note& note, bool& note_finished) override;
			};

			struct hihat : base
			{
				hihat();
				fwave_t sound(fwave_t time_global, const note& note, bool& note_finished) override;
			};
		}
	}

	struct sequencer
	{
	public:
		struct channel
		{
			inst::base* instrument;
			string_t beat;
		};

		sequencer(fwave_t tempo = 120.0, uint32_t beats = 4, uint32_t sub_beats = 4);

		uint32_t update(fwave_t deltaTime);
		void add_inst(inst::base* instrument, const string_t& beat = string_t());

	public:
		uint32_t beats;
		uint32_t subBeats;

		uint32_t currentBeat;
		uint32_t totalBeats;

		fwave_t tempo;
		fwave_t beatTime;
		fwave_t timeTotal;

		std::vector<note> notes;
		std::vector<channel> channels;

	};

	class base
	{
	public:
		base();

		std::list<note> notes;
		std::mutex mux;

		fwave_t sample(fwave_t time_global);

	};

#ifdef SYNTH_IMPL
#undef SYNTH_IMPL

	fwave_t ang(fwave_t freq)
	{
		return 2.0 * PI * freq;
	}

	fwave_t osc(fwave_t time, fwave_t hertz, oscillator oscillator, fwave_t lfo_hertz, fwave_t lfo_amplitude, fwave_t saw_period)
	{
		// Calculating frequency with modulation
		fwave_t freq = ang(hertz) * time + lfo_amplitude * hertz * wave(ang(lfo_hertz) * time);
		fwave_t output = 0.0;

		switch (oscillator)
		{
		case oscillator::SINE:
			output = wave(freq);
			break;

		case oscillator::TRIANGLE:
			output = asin(wave(freq)) * 2.0 / PI;
			break;

		case oscillator::SQUARE:
			output = wave(freq) > 0.0 ? 1.0 : -1.0;
			break;

		case oscillator::SAW_ANA:
		{
			for (fwave_t k = 1.0; k < saw_period; k += 1.0) output += wave(freq * k) / k;
			output *= 2.0 / PI;
		}
		break;

		case oscillator::SAW_DIG:
			output = freq * fmod(time, 1.0 / hertz) / PI / time - PI * 0.5;
			break;

		case oscillator::NOISE:
			output = 2.0 * (fwave_t)rand() / (fwave_t)RAND_MAX - 1.0;
			break;

		}

		return output;
	}

	note::note()
	{
		id = 0;
		on = 0.0;
		off = 0.0;
		active = false;
		channel = nullptr;
	}

	fwave_t scale(int32_t noteID)
	{
		return 8.0 * pow(TWLTH_ROOT_OF_2, noteID);
	}

	envelope::adsr::adsr()
	{
		attackTime = 0.1;
		decayTime = 0.1;
		sustainAmplitude = 1.0;
		releaseTime = 0.2;
		startAmplitude = 1.0;
	}

	fwave_t envelope::adsr::get_amplitude(fwave_t time_global, fwave_t time_on, fwave_t time_off)
	{
		fwave_t amplitude = 0.0;

		if (time_on > time_off) // Note is on
		{
			fwave_t lifeTime = time_global - time_on;

			if (lifeTime <= attackTime)
				amplitude = lifeTime / attackTime * startAmplitude;

			if (attackTime < lifeTime && lifeTime <= attackTime + decayTime)
				amplitude = (lifeTime - attackTime) / decayTime * (sustainAmplitude - startAmplitude) + startAmplitude;

			if (attackTime + decayTime < lifeTime)
				amplitude = sustainAmplitude;
		}
		else // Note is off
		{
			fwave_t lifeTime = time_off - time_on;
			fwave_t releaseAmplitude = 0.0;

			if (lifeTime <= attackTime)
				releaseAmplitude = lifeTime / attackTime * startAmplitude;

			if (attackTime < lifeTime && lifeTime <= attackTime + decayTime)
				releaseAmplitude = (lifeTime - attackTime) / decayTime * (sustainAmplitude - startAmplitude) + startAmplitude;

			if (attackTime + decayTime < lifeTime)
				releaseAmplitude = sustainAmplitude;

			amplitude = (time_global - time_off) / releaseTime * -releaseAmplitude + releaseAmplitude;
		}

		return amplitude > 0.01 ? amplitude : 0.0;
	}

	filter::base::base(fwave_t freq, fwave_t hertz)
	{
		alpha = 0.0;
		prevSample = 0.0;
		this->hertz = hertz;
		set_frequency(freq);
	}

	void filter::base::set_frequency(fwave_t freq)
	{
		alpha = exp(-2.0 * PI * freq / hertz);
	}

	filter::low_pass::low_pass(fwave_t freq, fwave_t hertz) : filter::base(freq, hertz)
	{
	}

	fwave_t filter::low_pass::filter(fwave_t time_global, fwave_t sample)
	{
		fwave_t output = (1.0 - alpha) * sample + alpha * prevSample;
		prevSample = output;
		return output;
	}

	filter::high_pass::high_pass(fwave_t freq, fwave_t hertz) : filter::base(freq, hertz)
	{
	}

	fwave_t filter::high_pass::filter(fwave_t time_global, fwave_t sample)
	{
		fwave_t output = alpha * (2.0 * prevSample - sample);
		prevSample = output;
		return output;
	}

	bool inst::base::is_note_finished(fwave_t amplitude, bool check_amplitude, fwave_t time_global, fwave_t time_on)
	{
		if (check_amplitude) return amplitude <= 0.0;
		return maxLifeTime > 0.0 && time_global - time_on >= maxLifeTime;
	}

	inst::bell::bell()
	{
		env.attackTime = 0.01;
		env.decayTime = 1.0;
		env.sustainAmplitude = 0.0;
		env.releaseTime = 1.0;

		volume = 1.0;
		maxLifeTime = 3.0;
		name = _T("Bell");
	}

	fwave_t inst::bell::sound(fwave_t time_global, const note& note, bool& note_finished)
	{
		fwave_t amplitude = env.get_amplitude(time_global, note.on, note.off);
		note_finished = is_note_finished(amplitude);

		fwave_t sound =
			1.0 * osc(time_global - note.on, scale(note.id + 12), oscillator::SINE, 5.0, 0.001) +
			0.5 * osc(time_global - note.on, scale(note.id + 24), oscillator::SINE) +
			0.25 * osc(time_global - note.on, scale(note.id + 36), oscillator::SINE);

		return amplitude * sound * volume;
	}

	inst::harmonica::harmonica()
	{
		env.attackTime = 0.0;
		env.decayTime = 1.0;
		env.sustainAmplitude = 0.95;
		env.releaseTime = 0.1;

		volume = 0.3;
		maxLifeTime = -1.0;
		name = _T("Harmonica");
	}

	fwave_t inst::harmonica::sound(fwave_t time_global, const note& note, bool& bNoteFinished)
	{
		fwave_t amplitude = env.get_amplitude(time_global, note.on, note.off);
		bNoteFinished = is_note_finished(amplitude);

		fwave_t sound =
			1.0 * osc(time_global - note.on, scale(note.id - 12), oscillator::SAW_ANA, 5.0, 0.001, 100.0) +
			1.0 * osc(time_global - note.on, scale(note.id), oscillator::SQUARE, 5.0, 0.001) +
			0.5 * osc(time_global - note.on, scale(note.id + 12), oscillator::SQUARE) +
			0.05 * osc(time_global - note.on, scale(note.id + 24), oscillator::NOISE);

		return amplitude * sound * volume;
	}

	inst::drum::kick::kick()
	{
		env.attackTime = 0.01;
		env.decayTime = 0.15;
		env.sustainAmplitude = 0.0;
		env.releaseTime = 0.0;

		volume = 1.0;
		maxLifeTime = 1.5;
		name = _T("Drum Kick");
	}

	fwave_t inst::drum::kick::sound(fwave_t time_global, const note& note, bool& note_finished)
	{
		fwave_t amplitude = env.get_amplitude(time_global, note.on, note.off);
		note_finished = is_note_finished(amplitude, false, time_global, note.on);

		fwave_t sound =
			0.99 * osc(time_global - note.on, scale(note.id - 36), oscillator::SINE, 1.0, 1.0) +
			0.01 * osc(time_global - note.on, 0, oscillator::NOISE);

		return amplitude * sound * volume;
	}

	inst::drum::snare::snare()
	{
		env.attackTime = 0.0;
		env.decayTime = 0.2;
		env.sustainAmplitude = 0.0;
		env.releaseTime = 0.0;

		volume = 1.0;
		maxLifeTime = 1.0;
		name = _T("Drum Snare");
	}

	fwave_t inst::drum::snare::sound(fwave_t time_global, const note& note, bool& note_finished)
	{
		fwave_t amplitude = env.get_amplitude(time_global, note.on, note.off);
		note_finished = is_note_finished(amplitude, false, time_global, note.on);

		fwave_t sound =
			0.5 * osc(time_global - note.on, scale(note.id - 24), oscillator::SINE, 0.5, 1.0) +
			0.5 * osc(time_global - note.on, 0, oscillator::NOISE);

		return amplitude * sound * volume;
	}

	inst::drum::hihat::hihat()
	{
		env.attackTime = 0.01;
		env.decayTime = 0.05;
		env.sustainAmplitude = 0.0;
		env.releaseTime = 0.0;

		volume = 0.5;
		maxLifeTime = 1.5;
		name = _T("Drum HiHat");
	}

	fwave_t inst::drum::hihat::sound(fwave_t time_global, const note& note, bool& note_finished)
	{
		fwave_t amplitude = env.get_amplitude(time_global, note.on, note.off);
		note_finished = is_note_finished(amplitude, false, time_global, note.on);

		fwave_t sound =
			0.1 * osc(time_global - note.on, scale(note.id - 12), oscillator::SQUARE, 1.5, 1.0) +
			0.9 * osc(time_global - note.on, 0, oscillator::NOISE);

		return amplitude * sound * volume;
	}

	sequencer::sequencer(fwave_t tempo, uint32_t beats, uint32_t sub_beats)
	{
		this->beats = beats;
		subBeats = sub_beats;

		currentBeat = 0;
		totalBeats = beats * sub_beats;

		this->tempo = tempo;
		beatTime = 60.0 / tempo / (fwave_t)sub_beats;
		timeTotal = 0.0;
	}

	uint32_t sequencer::update(fwave_t delta_time)
	{
		notes.clear();

		timeTotal += delta_time;
		while (timeTotal >= beatTime)
		{
			timeTotal -= beatTime;

			if (++currentBeat >= totalBeats)
				currentBeat = 0;

			for (auto& channel : channels)
			{
				if (channel.beat[currentBeat] == L'x')
				{
					note note;
					note.channel = channel.instrument;
					note.active = true;
					note.id = 64;
					notes.push_back(note);
				}
			}
		}

		return notes.size();
	}

	void sequencer::add_inst(inst::base* instrument, const string_t& beat)
	{
		channel channel;
		channel.instrument = instrument;
		channel.beat = beat;
		channels.push_back(channel);
	}

	base::base() {}

	fwave_t base::sample(fwave_t time_global)
	{
		std::unique_lock<std::mutex> lock(mux);

		fwave_t output = 0.0;

		for (auto& note : notes)
		{
			bool noteFinished = false;

			if (note.channel)
				output += note.channel->sound(time_global, note, noteFinished);

			if (noteFinished)
				note.active = false;
		}

		notes.remove_if([](const note& note) { return !note.active; });

		return output;
	}

#endif
}

#endif
