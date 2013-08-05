/*
*
* Author: Giacomo Lozito <james@develia.org>, (C) 2005-2006
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*
*/

#include <string.h>

#include <audacious/i18n.h>
#include <audacious/misc.h>

#include "../i_backend.h"
#include "../i_configure.h"
#include "b-fluidsynth.h"

/* sequencer instance */
static sequencer_client_t sc;
/* options */
static amidiplug_cfg_fsyn_t * fsyn_cfg;

int backend_init (amidiplug_cfg_backend_t * cfg)
{
    fsyn_cfg = cfg->fsyn;

    sc.soundfont_ids = g_array_new (FALSE, FALSE, sizeof (int));
    sc.settings = new_fluid_settings();

    fluid_settings_setnum (sc.settings, "synth.sample-rate", fsyn_cfg->fsyn_synth_samplerate);

    if (fsyn_cfg->fsyn_synth_gain != -1)
        fluid_settings_setnum (sc.settings, "synth.gain", (gdouble) fsyn_cfg->fsyn_synth_gain / 10);

    if (fsyn_cfg->fsyn_synth_polyphony != -1)
        fluid_settings_setint (sc.settings, "synth.polyphony", fsyn_cfg->fsyn_synth_polyphony);

    if (fsyn_cfg->fsyn_synth_reverb == 1)
        fluid_settings_setstr (sc.settings, "synth.reverb.active", "yes");
    else if (fsyn_cfg->fsyn_synth_reverb == 0)
        fluid_settings_setstr (sc.settings, "synth.reverb.active", "no");

    if (fsyn_cfg->fsyn_synth_chorus == 1)
        fluid_settings_setstr (sc.settings, "synth.chorus.active", "yes");
    else if (fsyn_cfg->fsyn_synth_chorus == 0)
        fluid_settings_setstr (sc.settings, "synth.chorus.active", "no");

    sc.synth = new_fluid_synth (sc.settings);

    /* soundfont loader, check if we should load soundfont on backend init */
    if (fsyn_cfg->fsyn_soundfont_load == 0)
        i_soundfont_load();

    return 1;
}


int backend_cleanup (void)
{
    if (sc.soundfont_ids->len > 0)
    {
        /* unload soundfonts */
        int i = 0;

        for (i = 0 ; i < sc.soundfont_ids->len ; i++)
            fluid_synth_sfunload (sc.synth, g_array_index (sc.soundfont_ids, int, i), 0);
    }

    g_array_free (sc.soundfont_ids, TRUE);
    delete_fluid_synth (sc.synth);
    delete_fluid_settings (sc.settings);

    return 1;
}


int sequencer_start (const char * midi_fname)
{
    /* soundfont loader, check if we should load soundfont on first midifile play */
    if ((fsyn_cfg->fsyn_soundfont_load == 1) && (sc.soundfont_ids->len == 0))
        i_soundfont_load();

    return 1; /* success */
}


int sequencer_stop (void)
{
    return 1; /* success */
}


/* activate sequencer client */
int sequencer_on (void)
{
    return 1; /* success */
}


/* shutdown sequencer client */
int sequencer_off (void)
{
    return 1; /* success */
}


/* queue set tempo */
int sequencer_queue_tempo (int tempo, int ppq)
{
    return 1;
}


int sequencer_queue_start (void)
{
    return 1;
}


int sequencer_queue_stop (void)
{
    return 1;
}


int sequencer_event_init (void)
{
    /* common settings for all our events */
    return 1;
}


int sequencer_event_noteon (midievent_t * event)
{
    fluid_synth_noteon (sc.synth,
                        event->data.d[0],
                        event->data.d[1],
                        event->data.d[2]);
    return 1;
}


int sequencer_event_noteoff (midievent_t * event)
{
    fluid_synth_noteoff (sc.synth,
                         event->data.d[0],
                         event->data.d[1]);
    return 1;
}


int sequencer_event_keypress (midievent_t * event)
{
    /* KEY PRESSURE events are not handled by FluidSynth sequencer? */
    DEBUGMSG ("KEYPRESS EVENT with FluidSynth backend (unhandled)\n");
    return 1;
}


int sequencer_event_controller (midievent_t * event)
{
    fluid_synth_cc (sc.synth,
                    event->data.d[0],
                    event->data.d[1],
                    event->data.d[2]);
    return 1;
}


int sequencer_event_pgmchange (midievent_t * event)
{
    fluid_synth_program_change (sc.synth,
                                event->data.d[0],
                                event->data.d[1]);
    return 1;
}


int sequencer_event_chanpress (midievent_t * event)
{
    /* CHANNEL PRESSURE events are not handled by FluidSynth sequencer? */
    DEBUGMSG ("CHANPRESS EVENT with FluidSynth backend (unhandled)\n");
    return 1;
}


int sequencer_event_pitchbend (midievent_t * event)
{
    int pb_value = (( (event->data.d[2]) & 0x7f) << 7) | ((event->data.d[1]) & 0x7f);
    fluid_synth_pitch_bend (sc.synth,
                            event->data.d[0],
                            pb_value);
    return 1;
}


int sequencer_event_sysex (midievent_t * event)
{
    DEBUGMSG ("SYSEX EVENT with FluidSynth backend (unhandled)\n");
    return 1;
}


int sequencer_event_tempo (midievent_t * event)
{
    return 1;
}


int sequencer_event_other (midievent_t * event)
{
    /* unhandled */
    return 1;
}


int sequencer_event_allnoteoff (int unused)
{
    int c = 0;

    for (c = 0 ; c < 16 ; c++)
    {
        fluid_synth_cc (sc.synth, c, 123 /* all notes off */, 0);
    }

    return 1;
}


static void generate_audio (void * buf, int bufsize)
{
    fluid_synth_write_s16 (sc.synth, bufsize / 4, buf, 0, 2, buf, 1, 2);
}

int sequencer_output_shut (unsigned max_tick, int skip_offset)
{
    fluid_synth_system_reset (sc.synth);  /* all notes off and channels reset */
    return 1;
}


int audio_info_get (int * channels, int * bitdepth, int * samplerate)
{
    *channels = 2;
    *bitdepth = 16; /* always 16 bit, we use fluid_synth_write_s16() */
    *samplerate = fsyn_cfg->fsyn_synth_samplerate;
    return 1; /* valid information */
}


/* ******************************************************************
   *** INTERNALS ****************************************************
   ****************************************************************** */

void i_soundfont_load (void)
{
    if (strcmp (fsyn_cfg->fsyn_soundfont_file, ""))
    {
        char ** sffiles = g_strsplit (fsyn_cfg->fsyn_soundfont_file, ";", 0);
        int i = 0;

        while (sffiles[i] != NULL)
        {
            int sf_id = 0;
            DEBUGMSG ("loading soundfont %s\n", sffiles[i]);
            sf_id = fluid_synth_sfload (sc.synth, sffiles[i], 0);

            if (sf_id == -1)
            {
                g_warning ("unable to load SoundFont file %s\n", sffiles[i]);
            }
            else
            {
                DEBUGMSG ("soundfont %s successfully loaded\n", sffiles[i]);
                g_array_append_val (sc.soundfont_ids, sf_id);
            }

            i++;
        }

        g_strfreev (sffiles);

        fluid_synth_system_reset (sc.synth);
    }
    else
    {
        g_warning ("FluidSynth backend was selected, but no SoundFont has been specified\n");
    }
}

amidiplug_sequencer_backend_t fluidsynth_backend =
{
    .init = backend_init,
    .cleanup = backend_cleanup,
    .audio_info_get = audio_info_get,
    .seq_start = sequencer_start,
    .seq_stop = sequencer_stop,
    .seq_on = sequencer_on,
    .seq_off = sequencer_off,
    .seq_queue_tempo = sequencer_queue_tempo,
    .seq_queue_start = sequencer_queue_start,
    .seq_queue_stop = sequencer_queue_stop,
    .seq_event_init = sequencer_event_init,
    .seq_event_noteon = sequencer_event_noteon,
    .seq_event_noteoff = sequencer_event_noteoff,
    .seq_event_allnoteoff = sequencer_event_allnoteoff,
    .seq_event_keypress = sequencer_event_keypress,
    .seq_event_controller = sequencer_event_controller,
    .seq_event_pgmchange = sequencer_event_pgmchange,
    .seq_event_chanpress = sequencer_event_chanpress,
    .seq_event_pitchbend = sequencer_event_pitchbend,
    .seq_event_sysex = sequencer_event_sysex,
    .seq_event_tempo = sequencer_event_tempo,
    .seq_event_other = sequencer_event_other,
    .generate_audio = generate_audio
};
