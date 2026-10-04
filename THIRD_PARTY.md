# Third-party code and artwork

This port builds on the [soloader boilerplate](https://github.com/v-atamanenko/soloader-boilerplate)
and compatibility work from [Scribblenauts Remix Vita](https://github.com/jwfeniello/scribblenauts-remix-vita).
Existing copyright and license notices are preserved in the source files and
the root `LICENSE`.

The following dependencies are vendored as source, including local changes.
Unrelated vitaGL sample programs and their media are omitted from this port:

| Dependency | Upstream snapshot | License file |
| --- | --- | --- |
| [FalsoJNI](https://github.com/v-atamanenko/FalsoJNI) | `083d5a07e025c6dfbb54ac68fd1acf696c23a86c` | `lib/falso_jni/LICENSE` |
| [FalsoNDK](https://github.com/elliencode/FalsoNDK) | `7dceb3bb34de5c71a3c5b98ed30c57a0eb53c73a` | `lib/falso_ndk/LICENSE` |
| [so_util](https://github.com/Rinnegatamante/so_util) | `c4732373e33d808cd885dec3c2c75302cc4c739b` | `lib/so_util/LICENSE` |
| [vitaGL](https://github.com/Rinnegatamante/vitaGL) | `9c23758ff17893db63887f95e9a4d9350c986d88` | `lib/vitagl/COPYING`, `lib/vitagl/COPYING.LESSER` |

FalsoJNI includes UTF-8 fixes and host-test adapters. vitaGL includes inherited
four-texture-stage changes. The corresponding patches are in `patches/`.
Additional compatibility sources under `lib/` and `source/` retain their
original notices; consult those files for their terms.

The original Amazing Alex icon, logo, character and promotional art supplied
for the launcher remain the property of their respective owners. The workshop
background was composed using built-in image generation with those references.
Source images and generation prompts are retained under
`extras/livearea/source/`, with the supplied references in
`liveareassetsnstiff/`. These artwork assets are not covered by the port's
source-code license.

The Android APK, native game library and playable game data are not included.
