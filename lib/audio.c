#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define SAMPLE_RATE 44100
#define CHANNELS 2
#define BITS_PER_SAMPLE 16
#define BUFFER_SIZE 4096
#define MAX_SOUNDS 32
#define MAX_EFFECTS 16

typedef struct {
    uint32_t id;
    char name[64];
    uint8_t* data;
    uint32_t size;
    uint32_t position;
    bool playing;
    bool looping;
    float volume;
    float pan;
    float speed;
} sound_t;

typedef struct {
    uint32_t id;
    char name[64];
    void (*process)(int16_t* buffer, uint32_t size, void* params);
    void* params;
} effect_t;

typedef struct {
    sound_t sounds[MAX_SOUNDS];
    uint32_t sound_count;
    effect_t effects[MAX_EFFECTS];
    uint32_t effect_count;
    int16_t* buffer;
    uint32_t buffer_position;
    bool initialized;
} audio_t;

static audio_t audio;

void init_audio() {
    memset(&audio, 0, sizeof(audio_t));
    audio.buffer = (int16_t*)kmalloc(BUFFER_SIZE * sizeof(int16_t));
    audio.initialized = true;
}

void cleanup_audio() {
    if (audio.initialized) {
        for (uint32_t i = 0; i < audio.sound_count; i++) {
            kfree(audio.sounds[i].data);
        }
        kfree(audio.buffer);
        audio.initialized = false;
    }
}

uint32_t load_sound(const char* name, const uint8_t* data, uint32_t size) {
    if (!audio.initialized || audio.sound_count >= MAX_SOUNDS) return 0;

    sound_t* sound = &audio.sounds[audio.sound_count++];
    sound->id = audio.sound_count;
    strncpy(sound->name, name, sizeof(sound->name) - 1);
    sound->data = (uint8_t*)kmalloc(size);
    memcpy(sound->data, data, size);
    sound->size = size;
    sound->position = 0;
    sound->playing = false;
    sound->looping = false;
    sound->volume = 1.0f;
    sound->pan = 0.0f;
    sound->speed = 1.0f;

    return sound->id;
}

void unload_sound(uint32_t sound_id) {
    if (sound_id > 0 && sound_id <= audio.sound_count) {
        kfree(audio.sounds[sound_id - 1].data);
        audio.sounds[sound_id - 1] = audio.sounds[--audio.sound_count];
    }
}

void play_sound(uint32_t sound_id) {
    if (sound_id > 0 && sound_id <= audio.sound_count) {
        audio.sounds[sound_id - 1].playing = true;
        audio.sounds[sound_id - 1].position = 0;
    }
}

void stop_sound(uint32_t sound_id) {
    if (sound_id > 0 && sound_id <= audio.sound_count) {
        audio.sounds[sound_id - 1].playing = false;
    }
}

void pause_sound(uint32_t sound_id) {
    if (sound_id > 0 && sound_id <= audio.sound_count) {
        audio.sounds[sound_id - 1].playing = false;
    }
}

void resume_sound(uint32_t sound_id) {
    if (sound_id > 0 && sound_id <= audio.sound_count) {
        audio.sounds[sound_id - 1].playing = true;
    }
}

void set_sound_volume(uint32_t sound_id, float volume) {
    if (sound_id > 0 && sound_id <= audio.sound_count) {
        audio.sounds[sound_id - 1].volume = volume;
    }
}

void set_sound_pan(uint32_t sound_id, float pan) {
    if (sound_id > 0 && sound_id <= audio.sound_count) {
        audio.sounds[sound_id - 1].pan = pan;
    }
}

void set_sound_speed(uint32_t sound_id, float speed) {
    if (sound_id > 0 && sound_id <= audio.sound_count) {
        audio.sounds[sound_id - 1].speed = speed;
    }
}

void set_sound_loop(uint32_t sound_id, bool loop) {
    if (sound_id > 0 && sound_id <= audio.sound_count) {
        audio.sounds[sound_id - 1].looping = loop;
    }
}

uint32_t add_effect(const char* name, void (*process)(int16_t*, uint32_t, void*), void* params) {
    if (!audio.initialized || audio.effect_count >= MAX_EFFECTS) return 0;

    effect_t* effect = &audio.effects[audio.effect_count++];
    effect->id = audio.effect_count;
    strncpy(effect->name, name, sizeof(effect->name) - 1);
    effect->process = process;
    effect->params = params;

    return effect->id;
}

void remove_effect(uint32_t effect_id) {
    if (effect_id > 0 && effect_id <= audio.effect_count) {
        audio.effects[effect_id - 1] = audio.effects[--audio.effect_count];
    }
}

void process_audio() {
    if (!audio.initialized) return;

    memset(audio.buffer, 0, BUFFER_SIZE * sizeof(int16_t));

    for (uint32_t i = 0; i < audio.sound_count; i++) {
        sound_t* sound = &audio.sounds[i];
        if (sound->playing) {
            uint32_t samples_to_process = BUFFER_SIZE / CHANNELS;
            uint32_t samples_processed = 0;

            while (samples_processed < samples_to_process) {
                if (sound->position >= sound->size) {
                    if (sound->looping) {
                        sound->position = 0;
                    } else {
                        sound->playing = false;
                        break;
                    }
                }

                int16_t sample = *(int16_t*)(sound->data + sound->position);
                float left_volume = sound->volume * (1.0f - sound->pan);
                float right_volume = sound->volume * (1.0f + sound->pan);

                audio.buffer[samples_processed * CHANNELS] += (int16_t)(sample * left_volume);
                audio.buffer[samples_processed * CHANNELS + 1] += (int16_t)(sample * right_volume);

                sound->position += sizeof(int16_t);
                samples_processed++;
            }
        }
    }

    for (uint32_t i = 0; i < audio.effect_count; i++) {
        if (audio.effects[i].process) {
            audio.effects[i].process(audio.buffer, BUFFER_SIZE, audio.effects[i].params);
        }
    }
}

void apply_reverb(int16_t* buffer, uint32_t size, void* params) {
    float* reverb_params = (float*)params;
    float delay = reverb_params[0];
    float decay = reverb_params[1];
    float wet = reverb_params[2];
    float dry = reverb_params[3];

    uint32_t delay_samples = (uint32_t)(delay * SAMPLE_RATE);
    int16_t* delay_buffer = (int16_t*)kmalloc(delay_samples * sizeof(int16_t));
    memset(delay_buffer, 0, delay_samples * sizeof(int16_t));

    for (uint32_t i = 0; i < size; i++) {
        int16_t input = buffer[i];
        int16_t delayed = delay_buffer[i % delay_samples];
        buffer[i] = (int16_t)(input * dry + delayed * wet);
        delay_buffer[i % delay_samples] = (int16_t)(input + delayed * decay);
    }

    kfree(delay_buffer);
}

void apply_echo(int16_t* buffer, uint32_t size, void* params) {
    float* echo_params = (float*)params;
    float delay = echo_params[0];
    float feedback = echo_params[1];
    float wet = echo_params[2];
    float dry = echo_params[3];

    uint32_t delay_samples = (uint32_t)(delay * SAMPLE_RATE);
    int16_t* delay_buffer = (int16_t*)kmalloc(delay_samples * sizeof(int16_t));
    memset(delay_buffer, 0, delay_samples * sizeof(int16_t));

    for (uint32_t i = 0; i < size; i++) {
        int16_t input = buffer[i];
        int16_t delayed = delay_buffer[i % delay_samples];
        buffer[i] = (int16_t)(input * dry + delayed * wet);
        delay_buffer[i % delay_samples] = (int16_t)(input + delayed * feedback);
    }

    kfree(delay_buffer);
}

void apply_compressor(int16_t* buffer, uint32_t size, void* params) {
    float* comp_params = (float*)params;
    float threshold = comp_params[0];
    float ratio = comp_params[1];
    float attack = comp_params[2];
    float release = comp_params[3];

    float envelope = 0.0f;
    float attack_coef = exp(-1.0f / (attack * SAMPLE_RATE));
    float release_coef = exp(-1.0f / (release * SAMPLE_RATE));

    for (uint32_t i = 0; i < size; i++) {
        float input = buffer[i] / 32768.0f;
        float input_abs = fabs(input);
        
        if (input_abs > envelope) {
            envelope = attack_coef * envelope + (1.0f - attack_coef) * input_abs;
        } else {
            envelope = release_coef * envelope + (1.0f - release_coef) * input_abs;
        }

        float gain = 1.0f;
        if (envelope > threshold) {
            gain = threshold + (envelope - threshold) / ratio;
            gain = gain / envelope;
        }

        buffer[i] = (int16_t)(input * gain * 32768.0f);
    }
}

void apply_equalizer(int16_t* buffer, uint32_t size, void* params) {
    float* eq_params = (float*)params;
    float low_gain = eq_params[0];
    float mid_gain = eq_params[1];
    float high_gain = eq_params[2];

    float low_coef = 0.1f;
    float mid_coef = 0.5f;
    float high_coef = 0.9f;

    float low_prev = 0.0f;
    float mid_prev = 0.0f;
    float high_prev = 0.0f;

    for (uint32_t i = 0; i < size; i++) {
        float input = buffer[i] / 32768.0f;
        
        float low = low_coef * input + (1.0f - low_coef) * low_prev;
        float mid = mid_coef * input + (1.0f - mid_coef) * mid_prev;
        float high = high_coef * input + (1.0f - high_coef) * high_prev;

        low_prev = low;
        mid_prev = mid;
        high_prev = high;

        float output = low * low_gain + mid * mid_gain + high * high_gain;
        buffer[i] = (int16_t)(output * 32768.0f);
    }
} 