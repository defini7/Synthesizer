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

#ifdef SYNTH_APPROX
FTYPE approxsin(FTYPE x)
{
	x *= 0.15915;
	x -= (int32_t)x;
	return 20.875 * x * (x - 0.5) * (x - 1.0);
}

// TODO: Approx asin

#define SYNTH_WAVE(x) approxsin(x)
#else
#define SYNTH_WAVE(x) sin(x)
#endif

namespace synth
{
	constexpr FTYPE PI = 3.14159;
	const FTYPE TWLTH_ROOT_OF_2 = pow(2.0, 1.0 / 12.0);

	enum class Oscillator : uint8_t
	{
		SINE,
		TRIANGLE,
		SQUARE,
		SAW_ANA,
		SAW_DIG,
		NOISE
	};

	FTYPE Ang(FTYPE dFreq);
	FTYPE Osc(FTYPE dTime, FTYPE dHertz, Oscillator oscillator, FTYPE dLFOHertz = 0.0, FTYPE dLFOAmplitude = 0.0, FTYPE dSawPeriod = 50.0);

	namespace inst { struct Base; }

	struct Note
	{
		int32_t id;
		FTYPE on, off;
		bool active;

		inst::Base* channel;

		Note();
	};

	FTYPE Scale(FTYPE dFreq, int32_t nNoteID);

	namespace envelope
	{
		struct Base
		{
			virtual FTYPE GetAmplitude(FTYPE dTimeGlobal, FTYPE dTimeOn, FTYPE dTimeOff) = 0;
		};

		struct ADSR : Base
		{
			FTYPE dAttackTime;
			FTYPE dDecayTime;
			FTYPE dSustainAmplitude;
			FTYPE dReleaseTime;
			FTYPE dStartAmplitude;

			ADSR();
			FTYPE GetAmplitude(FTYPE dTimeGlobal, FTYPE dTimeOn, FTYPE dTimeOff) override;
		};
	}

	namespace filter
	{
		struct Base
		{
			FTYPE dAlpha;
			FTYPE dPrevSample;
			FTYPE dHertz;

			Base(FTYPE dFreq = 120.0, FTYPE dHertz = 44100.0);

			void SetFrequency(FTYPE dFreq);
			virtual FTYPE GetFilter(FTYPE dTimeGlobal, FTYPE dSample) = 0;
		};

		struct LowPass : Base
		{
			LowPass(FTYPE dFreq = 120.0, FTYPE dHertz = 44100.0);

			virtual FTYPE GetFilter(FTYPE dTimeGlobal, FTYPE dSample) override;
		};

		struct HighPass : Base
		{
			HighPass(FTYPE dFreq = 120.0, FTYPE dHertz = 44100.0);

			virtual FTYPE GetFilter(FTYPE dTimeGlobal, FTYPE dSample) override;
		};
	}

	namespace inst
	{
		struct Base
		{
			FTYPE dVolume;
			FTYPE dMaxLifeTime;
			std::wstring sName;

			envelope::ADSR env;

			virtual FTYPE Sound(FTYPE dTimeGlobal, const Note& note, bool& bNoteFinished) = 0;

			bool NoteFinished(FTYPE dAmplitude, bool bCheckAmplitude = true, FTYPE dTimeGlobal = 0.0, FTYPE dTimeOn = 0.0);
		};

		struct Bell : Base
		{
			Bell();
			FTYPE Sound(FTYPE dTimeGlobal, const Note& note, bool& bNoteFinished) override;
		};

		struct Harmonica : Base
		{
			Harmonica();
			FTYPE Sound(FTYPE dTimeGlobal, const Note& note, bool& bNoteFinished) override;
		};

		namespace drum
		{
			struct Kick : Base
			{
				Kick();
				FTYPE Sound(FTYPE dTimeGlobal, const Note& note, bool& bNoteFinished) override;
			};

			struct Snare : Base
			{
				Snare();
				FTYPE Sound(FTYPE dTimeGlobal, const Note& note, bool& bNoteFinished) override;
			};

			struct HiHat : Base
			{
				HiHat();
				FTYPE Sound(FTYPE dTimeGlobal, const Note& note, bool& bNoteFinished) override;
			};
		}
	}

	struct Sequencer
	{
	public:
		struct Channel
		{
			inst::Base* pInstrument;
			std::wstring sBeat;
		};
		
		Sequencer(FTYPE dTempo = 120.0, uint32_t nBeats = 4, uint32_t nSubBeats = 4);

		uint32_t Update(FTYPE dDeltaTime);
		void AddInst(inst::Base* pInstrument, const std::wstring& sBeat = L"");

	public:
		uint32_t nBeats;
		uint32_t nSubBeats;

		uint32_t nCurrentBeat;
		uint32_t nTotalBeats;

		FTYPE dTempo;
		FTYPE dBeatTime;
		FTYPE dTimeTotal;

		std::vector<Note> vNotes;
		std::vector<Channel> vChannels;
		
	};

	class Base
	{
	public:
		Base();

		std::list<Note> listNotes;
		std::mutex muxNotes;

		FTYPE GetSample(FTYPE dTimeGlobal);

	};

#ifdef SYNTH_IMPL
#undef SYNTH_IMPL

	FTYPE Ang(FTYPE dFreq)
	{
		return 2.0 * PI * dFreq;
	}

	FTYPE Osc(FTYPE dTime, FTYPE dHertz, Oscillator oscillator, FTYPE dLFOHertz, FTYPE dLFOAmplitude, FTYPE dSawPeriod)
	{
		// Calculating frequency with modulation
		FTYPE dFreq = Ang(dHertz) * dTime + dLFOAmplitude * dHertz * SYNTH_WAVE(Ang(dLFOHertz) * dTime);
		FTYPE dOutput = 0.0;

		switch (oscillator)
		{
		case Oscillator::SINE:
			dOutput = SYNTH_WAVE(dFreq);
		break;

		case Oscillator::TRIANGLE:
			dOutput = asin(SYNTH_WAVE(dFreq)) * 2.0 / PI;
		break;

		case Oscillator::SQUARE:
			dOutput = SYNTH_WAVE(dFreq) > 0.0 ? 1.0 : -1.0;
		break;

		case Oscillator::SAW_ANA:
		{
			for (FTYPE k = 1.0; k < dSawPeriod; k += 1.0) dOutput += SYNTH_WAVE(dFreq * k) / k;
			dOutput *= 2.0 / PI;
		}
		break;

		case Oscillator::SAW_DIG:
			dOutput = dFreq * fmod(dTime, 1.0 / dHertz) / PI / dTime - PI * 0.5;
		break;

		case Oscillator::NOISE:
			dOutput = 2.0 * (FTYPE)rand() / (FTYPE)RAND_MAX - 1.0;
		break;

		}

		return dOutput;
	}

	Note::Note()
	{
		id = 0;
		on = 0.0;
		off = 0.0;
		active = false;
		channel = nullptr;
	}

	FTYPE Scale(int32_t nNoteID)	
	{
		return 8.0 * pow(TWLTH_ROOT_OF_2, nNoteID);
	}

	envelope::ADSR::ADSR()
	{
		dAttackTime = 0.1;
		dDecayTime = 0.1;
		dSustainAmplitude = 1.0;
		dReleaseTime = 0.2;
		dStartAmplitude = 1.0;
	}

	FTYPE envelope::ADSR::GetAmplitude(FTYPE dTimeGlobal, FTYPE dTimeOn, FTYPE dTimeOff)
	{
		FTYPE dAmplitude = 0.0;

		if (dTimeOn > dTimeOff) // Note is on
		{
			FTYPE dLifeTime = dTimeGlobal - dTimeOn;

			if (dLifeTime <= dAttackTime)
				dAmplitude = dLifeTime / dAttackTime * dStartAmplitude;

			if (dAttackTime < dLifeTime && dLifeTime <= dAttackTime + dDecayTime)
				dAmplitude = (dLifeTime - dAttackTime) / dDecayTime * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;

			if (dAttackTime + dDecayTime < dLifeTime)
				dAmplitude = dSustainAmplitude;
		}
		else // Note is off
		{
			FTYPE dLifeTime = dTimeOff - dTimeOn;
			FTYPE dReleaseAmplitude = 0.0;

			if (dLifeTime <= dAttackTime)
				dReleaseAmplitude = dLifeTime / dAttackTime * dStartAmplitude;

			if (dAttackTime < dLifeTime && dLifeTime <= dAttackTime + dDecayTime)
				dReleaseAmplitude = (dLifeTime - dAttackTime) / dDecayTime * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;

			if (dAttackTime + dDecayTime < dLifeTime)
				dReleaseAmplitude = dSustainAmplitude;

			dAmplitude = (dTimeGlobal - dTimeOff) / dReleaseTime * -dReleaseAmplitude + dReleaseAmplitude;
		}

		return dAmplitude > 0.01 ? dAmplitude : 0.0;
	}

	filter::Base::Base(FTYPE dFreq, FTYPE dHertz)
	{
		dAlpha = 0.0;
		dPrevSample = 0.0;
		this->dHertz = dHertz;
		SetFrequency(dFreq);
	}

	void filter::Base::SetFrequency(FTYPE dFreq)
	{
		dAlpha = exp(-2.0 * PI * dFreq / dHertz);
	}

	filter::LowPass::LowPass(FTYPE dFreq, FTYPE dHertz) : filter::Base(dFreq, dHertz)
	{
	}

	FTYPE filter::LowPass::GetFilter(FTYPE dTimeGlobal, FTYPE dSample)
	{
		FTYPE dOutput = (1.0 - dAlpha) * dSample + dAlpha * dPrevSample;
		dPrevSample = dOutput;
		return dOutput;
	}

	filter::HighPass::HighPass(FTYPE dFreq, FTYPE dHertz) : filter::Base(dFreq, dHertz)
	{
	}

	FTYPE filter::HighPass::GetFilter(FTYPE dTimeGlobal, FTYPE dSample)
	{
		FTYPE dOutput = dAlpha * (2.0 * dPrevSample - dSample);
		dPrevSample = dOutput;
		return dOutput;
	}

	bool inst::Base::NoteFinished(FTYPE dAmplitude, bool bCheckAmplitude, FTYPE dTimeGlobal, FTYPE dTimeOn)
	{
		if (bCheckAmplitude) return dAmplitude <= 0.0;
		return dMaxLifeTime > 0.0 && dTimeGlobal - dTimeOn >= dMaxLifeTime;
	}

	inst::Bell::Bell()
	{
		env.dAttackTime = 0.01;
		env.dDecayTime = 1.0;
		env.dSustainAmplitude = 0.0;
		env.dReleaseTime = 1.0;

		dVolume = 1.0;
		dMaxLifeTime = 3.0;
		sName = L"Bell";
	}

	FTYPE inst::Bell::Sound(FTYPE dTimeGlobal, const Note& note, bool& bNoteFinished)
	{
		FTYPE dAmplitude = env.GetAmplitude(dTimeGlobal, note.on, note.off);
		bNoteFinished = NoteFinished(dAmplitude);

		FTYPE dSound =
			1.0 * Osc(dTimeGlobal - note.on, Scale(note.id + 12), Oscillator::SINE, 5.0, 0.001) +
			0.5 * Osc(dTimeGlobal - note.on, Scale(note.id + 24), Oscillator::SINE) +
			0.25 * Osc(dTimeGlobal - note.on, Scale(note.id + 36), Oscillator::SINE);

		return dAmplitude * dSound * dVolume;
	}

	inst::Harmonica::Harmonica()
	{
		env.dAttackTime = 0.0;
		env.dDecayTime = 1.0;
		env.dSustainAmplitude = 0.95;
		env.dReleaseTime = 0.1;

		dVolume = 0.3;
		dMaxLifeTime = -1.0;
		sName = L"Harmonica";
	}

	FTYPE inst::Harmonica::Sound(FTYPE dTimeGlobal, const Note& note, bool& bNoteFinished)
	{
		FTYPE dAmplitude = env.GetAmplitude(dTimeGlobal, note.on, note.off);
		bNoteFinished = NoteFinished(dAmplitude);

		FTYPE dSound =
			1.0 * Osc(dTimeGlobal - note.on, Scale(note.id - 12), Oscillator::SAW_ANA, 5.0, 0.001, 100.0) +
			1.0 * Osc(dTimeGlobal - note.on, Scale(note.id), Oscillator::SQUARE, 5.0, 0.001) +
			0.5 * Osc(dTimeGlobal - note.on, Scale(note.id + 12), Oscillator::SQUARE) +
			0.05 * Osc(dTimeGlobal - note.on, Scale(note.id + 24), Oscillator::NOISE);

		return dAmplitude * dSound * dVolume;
	}

	inst::drum::Kick::Kick()
	{
		env.dAttackTime = 0.01;
		env.dDecayTime = 0.15;
		env.dSustainAmplitude = 0.0;
		env.dReleaseTime = 0.0;

		dVolume = 1.0;
		dMaxLifeTime = 1.5;
		sName = L"Drum Kick";
	}

	FTYPE inst::drum::Kick::Sound(FTYPE dTimeGlobal, const Note& note, bool& bNoteFinished)
	{
		FTYPE dAmplitude = env.GetAmplitude(dTimeGlobal, note.on, note.off);
		bNoteFinished = NoteFinished(dAmplitude, false, dTimeGlobal, note.on);

		FTYPE dSound =
			0.99 * Osc(dTimeGlobal - note.on, Scale(note.id - 36), Oscillator::SINE, 1.0, 1.0) +
			0.01 * Osc(dTimeGlobal - note.on, 0, Oscillator::NOISE);

		return dAmplitude * dSound * dVolume;
	}

	inst::drum::Snare::Snare()
	{
		env.dAttackTime = 0.0;
		env.dDecayTime = 0.2;
		env.dSustainAmplitude = 0.0;
		env.dReleaseTime = 0.0;

		dVolume = 1.0;
		dMaxLifeTime = 1.0;
		sName = L"Drum Snare";
	}

	FTYPE inst::drum::Snare::Sound(FTYPE dTimeGlobal, const Note& note, bool& bNoteFinished)
	{
		FTYPE dAmplitude = env.GetAmplitude(dTimeGlobal, note.on, note.off);
		bNoteFinished = NoteFinished(dAmplitude, false, dTimeGlobal, note.on);

		FTYPE dSound =
			0.5 * Osc(dTimeGlobal - note.on, Scale(note.id - 24), Oscillator::SINE, 0.5, 1.0) +
			0.5 * Osc(dTimeGlobal - note.on, 0, Oscillator::NOISE);

		return dAmplitude * dSound * dVolume;
	}

	inst::drum::HiHat::HiHat()
	{
		env.dAttackTime = 0.01;
		env.dDecayTime = 0.05;
		env.dSustainAmplitude = 0.0;
		env.dReleaseTime = 0.0;

		dVolume = 0.5;
		dMaxLifeTime = 1.5;
		sName = L"Drum HiHat";
	}

	FTYPE inst::drum::HiHat::Sound(FTYPE dTimeGlobal, const Note& note, bool& bNoteFinished)
	{
		FTYPE dAmplitude = env.GetAmplitude(dTimeGlobal, note.on, note.off);
		bNoteFinished = NoteFinished(dAmplitude, false, dTimeGlobal, note.on);

		FTYPE dSound =
			0.1 * Osc(dTimeGlobal - note.on, Scale(note.id - 12), Oscillator::SQUARE, 1.5, 1.0) +
			0.9 * Osc(dTimeGlobal - note.on, 0, Oscillator::NOISE);

		return dAmplitude * dSound * dVolume;
	}

	Sequencer::Sequencer(FTYPE dTempo, uint32_t nBeats, uint32_t nSubBeats)
	{
		this->nBeats = nBeats;
		this->nSubBeats = nSubBeats;

		nCurrentBeat = 0;
		nTotalBeats = nBeats * nSubBeats;

		this->dTempo = dTempo;
		dBeatTime = 60.0 / dTempo / (FTYPE)nSubBeats;
		dTimeTotal = 0.0;
	}

	uint32_t Sequencer::Update(FTYPE dDeltaTime)
	{
		vNotes.clear();

		dTimeTotal += dDeltaTime;
		while (dTimeTotal >= dBeatTime)
		{
			dTimeTotal -= dBeatTime;

			if (++nCurrentBeat >= nTotalBeats)
				nCurrentBeat = 0;

			for (auto& channel : vChannels)
			{
				if (channel.sBeat[nCurrentBeat] == L'x')
				{
					Note note;
					note.channel = channel.pInstrument;
					note.active = true;
					note.id = 64;
					vNotes.push_back(note);
				}
			}
		}

		return vNotes.size();
	}

	void Sequencer::AddInst(inst::Base* pInstrument, const std::wstring& sBeat)
	{
		Channel channel;
		channel.pInstrument = pInstrument;
		channel.sBeat = sBeat;
		vChannels.push_back(channel);
	}

	Base::Base() {}

	FTYPE Base::GetSample(FTYPE dTimeGlobal)
	{
		std::unique_lock<std::mutex> lock(muxNotes);

		FTYPE dOutput = 0.0;

		for (auto& note : listNotes)
		{
			bool bNoteFinished = false;

			if (note.channel)
				dOutput += note.channel->Sound(dTimeGlobal, note, bNoteFinished);

			if (bNoteFinished)
				note.active = false;
		}

		listNotes.remove_if([](const Note& note) { return !note.active; });

		return dOutput;
	}

#endif
}

#endif
