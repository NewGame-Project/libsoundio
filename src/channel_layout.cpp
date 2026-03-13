/*
 * Copyright (c) 2015 Andrew Kelley
 *
 * This file is part of libsoundio, which is MIT licensed.
 * See http://opensource.org/licenses/MIT
 */

#include "soundio_private.h"

#include <stdio.h>

static struct SoundIoChannelLayout builtin_channel_layouts[] = {
    {
        "Mono",
        1,
        {
            SoundIoChannelIdFrontCenter,
        },
    },
    {
        "Stereo",
        2,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
        },
    },
    {
        "2.1",
        3,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdLfe,
        },
    },
    {
        "3.0",
        3,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
        }
    },
    {
        "3.0 (back)",
        3,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdBackCenter,
        }
    },
    {
        "3.1",
        4,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdLfe,
        }
    },
    {
        "4.0",
        4,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdBackCenter,
        }
    },
    {
        "Quad",
        4,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdBackLeft,
            SoundIoChannelIdBackRight,
        },
    },
    {
        "Quad (side)",
        4,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdSideLeft,
            SoundIoChannelIdSideRight,
        }
    },
    {
        "4.1",
        5,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdBackCenter,
            SoundIoChannelIdLfe,
        }
    },
    {
        "5.0 (back)",
        5,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdBackLeft,
            SoundIoChannelIdBackRight,
        }
    },
    {
        "5.0 (side)",
        5,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdSideLeft,
            SoundIoChannelIdSideRight,
        }
    },
    {
        "5.1",
        6,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdSideLeft,
            SoundIoChannelIdSideRight,
            SoundIoChannelIdLfe,
        }
    },
    {
        "5.1 (back)",
        6,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdBackLeft,
            SoundIoChannelIdBackRight,
            SoundIoChannelIdLfe,
        }
    },
    {
        "6.0 (side)",
        6,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdSideLeft,
            SoundIoChannelIdSideRight,
            SoundIoChannelIdBackCenter,
        }
    },
    {
        "6.0 (front)",
        6,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdSideLeft,
            SoundIoChannelIdSideRight,
            SoundIoChannelIdFrontLeftCenter,
            SoundIoChannelIdFrontRightCenter,
        }
    },
    {
        "Hexagonal",
        6,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdBackLeft,
            SoundIoChannelIdBackRight,
            SoundIoChannelIdBackCenter,
        }
    },
    {
        "6.1",
        7,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdSideLeft,
            SoundIoChannelIdSideRight,
            SoundIoChannelIdBackCenter,
            SoundIoChannelIdLfe,
        }
    },
    {
        "6.1 (back)",
        7,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdBackLeft,
            SoundIoChannelIdBackRight,
            SoundIoChannelIdBackCenter,
            SoundIoChannelIdLfe,
        }
    },
    {
        "6.1 (front)",
        7,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdSideLeft,
            SoundIoChannelIdSideRight,
            SoundIoChannelIdFrontLeftCenter,
            SoundIoChannelIdFrontRightCenter,
            SoundIoChannelIdLfe,
        }
    },
    {
        "7.0",
        7,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdSideLeft,
            SoundIoChannelIdSideRight,
            SoundIoChannelIdBackLeft,
            SoundIoChannelIdBackRight,
        }
    },
    {
        "7.0 (front)",
        7,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdSideLeft,
            SoundIoChannelIdSideRight,
            SoundIoChannelIdFrontLeftCenter,
            SoundIoChannelIdFrontRightCenter,
        }
    },
    {
        "7.1",
        8,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdSideLeft,
            SoundIoChannelIdSideRight,
            SoundIoChannelIdBackLeft,
            SoundIoChannelIdBackRight,
            SoundIoChannelIdLfe,
        }
    },
    {
        "7.1 (wide)",
        8,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdSideLeft,
            SoundIoChannelIdSideRight,
            SoundIoChannelIdFrontLeftCenter,
            SoundIoChannelIdFrontRightCenter,
            SoundIoChannelIdLfe,
        }
    },
    {
        "7.1 (wide) (back)",
        8,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdBackLeft,
            SoundIoChannelIdBackRight,
            SoundIoChannelIdFrontLeftCenter,
            SoundIoChannelIdFrontRightCenter,
            SoundIoChannelIdLfe,
        }
    },
    {
        "Octagonal",
        8,
        {
            SoundIoChannelIdFrontLeft,
            SoundIoChannelIdFrontRight,
            SoundIoChannelIdFrontCenter,
            SoundIoChannelIdSideLeft,
            SoundIoChannelIdSideRight,
            SoundIoChannelIdBackLeft,
            SoundIoChannelIdBackRight,
            SoundIoChannelIdBackCenter,
        }
    },
};

#define CHANNEL_NAME_ALIAS_COUNT 3
typedef const std::wstring channel_names_t[CHANNEL_NAME_ALIAS_COUNT];
static channel_names_t channel_names[] = {
    {L"(Invalid Channel)", NULL, NULL},
    {L"Front Left", L"FL", L"front-left"},
    {L"Front Right", L"FR", L"front-right"},
    {L"Front Center", L"FC", L"front-center"},
    {L"LFE", L"LFE", L"lfe"},
    {L"Back Left", L"BL", L"rear-left"},
    {L"Back Right", L"BR", L"rear-right"},
    {L"Front Left Center", L"FLC", L"front-left-of-center"},
    {L"Front Right Center", L"FRC", L"front-right-of-center"},
    {L"Back Center", L"BC", L"rear-center"},
    {L"Side Left", L"SL", L"side-left"},
    {L"Side Right", L"SR", L"side-right"},
    {L"Top Center", L"TC", L"top-center"},
    {L"Top Front Left", L"TFL", L"top-front-left"},
    {L"Top Front Center", L"TFC", L"top-front-center"},
    {L"Top Front Right", L"TFR", L"top-front-right"},
    {L"Top Back Left", L"TBL", L"top-rear-left"},
    {L"Top Back Center", L"TBC", L"top-rear-center"},
    {L"Top Back Right", L"TBR", L"top-rear-right"},
    {L"Back Left Center", NULL, NULL},
    {L"Back Right Center", NULL, NULL},
    {L"Front Left Wide", NULL, NULL},
    {L"Front Right Wide", NULL, NULL},
    {L"Front Left High", NULL, NULL},
    {L"Front Center High", NULL, NULL},
    {L"Front Right High", NULL, NULL},
    {L"Top Front Left Center", NULL, NULL},
    {L"Top Front Right Center", NULL, NULL},
    {L"Top Side Left", NULL, NULL},
    {L"Top Side Right", NULL, NULL},
    {L"Left LFE", NULL, NULL},
    {L"Right LFE", NULL, NULL},
    {L"LFE 2", NULL, NULL},
    {L"Bottom Center", NULL, NULL},
    {L"Bottom Left Center", NULL, NULL},
    {L"Bottom Right Center", NULL, NULL},
    {L"Mid/Side Mid", NULL, NULL},
    {L"Mid/Side Side", NULL, NULL},
    {L"Ambisonic W", NULL, NULL},
    {L"Ambisonic X", NULL, NULL},
    {L"Ambisonic Y", NULL, NULL},
    {L"Ambisonic Z", NULL, NULL},
    {L"X-Y X", NULL, NULL},
    {L"X-Y Y", NULL, NULL},
    {L"Headphones Left", NULL, NULL},
    {L"Headphones Right", NULL, NULL},
    {L"Click Track", NULL, NULL},
    {L"Foreign Language", NULL, NULL},
    {L"Hearing Impaired", NULL, NULL},
    {L"Narration", NULL, NULL},
    {L"Haptic", NULL, NULL},
    {L"Dialog Centric Mix", NULL, NULL},
    {L"Aux", NULL, NULL},
    {L"Aux 0", NULL, NULL},
    {L"Aux 1", NULL, NULL},
    {L"Aux 2", NULL, NULL},
    {L"Aux 3", NULL, NULL},
    {L"Aux 4", NULL, NULL},
    {L"Aux 5", NULL, NULL},
    {L"Aux 6", NULL, NULL},
    {L"Aux 7", NULL, NULL},
    {L"Aux 8", NULL, NULL},
    {L"Aux 9", NULL, NULL},
    {L"Aux 10", NULL, NULL},
    {L"Aux 11", NULL, NULL},
    {L"Aux 12", NULL, NULL},
    {L"Aux 13", NULL, NULL},
    {L"Aux 14", NULL, NULL},
    {L"Aux 15", NULL, NULL},
};

const std::wstring soundio_get_channel_name(enum SoundIoChannelId id)
{
    if (id >= std::size(channel_names))
    {
        return L"(Invalid Channel)";
    }
    return channel_names[id][0];
}

bool soundio_channel_layout_equal(const struct SoundIoChannelLayout* a, const struct SoundIoChannelLayout* b)
{
    if (a->channel_count != b->channel_count)
        return false;

    for (int i = 0; i < a->channel_count; i += 1)
    {
        if (a->channels[i] != b->channels[i])
            return false;
    }

    return true;
}

int soundio_channel_layout_builtin_count()
{
    return std::size(builtin_channel_layouts);
}

const struct SoundIoChannelLayout* soundio_channel_layout_get_builtin(int index)
{
    assert(index >= 0);
    assert(index <= std::size(builtin_channel_layouts));
    return &builtin_channel_layouts[index];
}

int soundio_channel_layout_find_channel(
    const struct SoundIoChannelLayout* layout, enum SoundIoChannelId channel)
{
    for (int i = 0; i < layout->channel_count; i += 1)
    {
        if (layout->channels[i] == channel)
            return i;
    }
    return -1;
}

bool soundio_channel_layout_detect_builtin(struct SoundIoChannelLayout* layout)
{
    for (int i = 0; i < std::size(builtin_channel_layouts); i += 1)
    {
        const struct SoundIoChannelLayout* builtin_layout = &builtin_channel_layouts[i];
        if (soundio_channel_layout_equal(builtin_layout, layout))
        {
            layout->name = builtin_layout->name;
            return true;
        }
    }
    layout->name = NULL;
    return false;
}

const struct SoundIoChannelLayout* soundio_channel_layout_get_default(int channel_count)
{
    switch (channel_count)
    {
        case 1:
            return soundio_channel_layout_get_builtin(SoundIoChannelLayoutIdMono);
        case 2:
            return soundio_channel_layout_get_builtin(SoundIoChannelLayoutIdStereo);
        case 3:
            return soundio_channel_layout_get_builtin(SoundIoChannelLayoutId3Point0);
        case 4:
            return soundio_channel_layout_get_builtin(SoundIoChannelLayoutId4Point0);
        case 5:
            return soundio_channel_layout_get_builtin(SoundIoChannelLayoutId5Point0Back);
        case 6:
            return soundio_channel_layout_get_builtin(SoundIoChannelLayoutId5Point1Back);
        case 7:
            return soundio_channel_layout_get_builtin(SoundIoChannelLayoutId6Point1);
        case 8:
            return soundio_channel_layout_get_builtin(SoundIoChannelLayoutId7Point1);
    }
    return NULL;
}

enum SoundIoChannelId soundio_parse_channel_id(const std::wstring str)
{
    for (int id = 0; id < std::size(channel_names); id += 1)
    {
        for (int i = 0; i < CHANNEL_NAME_ALIAS_COUNT; i += 1)
        {
            const std::wstring alias = channel_names[id][i];
            if (alias.empty())
            {
                break;
            }

            if (alias == str)
            {
                return static_cast<SoundIoChannelId>(id);
            }
        }
    }
    return SoundIoChannelIdInvalid;
}
