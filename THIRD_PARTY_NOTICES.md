# Third-Party Notices

This file records dependencies and provenance checks. It is not a license grant
for this project or for assets whose origins have not been verified.
The repository-level policy is recorded in [Rights And Licensing](LICENSE.md).

## HMI Reference

The HMI was developed using
[Qt-HMI-Display-UI by cppqtdev](https://github.com/cppqtdev/Qt-HMI-Display-UI)
as a source/design reference. Repository availability alone does not establish
redistribution permission. On 2026-10-08 the upstream root directory contained
no LICENSE file, and no upstream license grant was confirmed. Retained reference
code and artwork are attributed here, not relicensed as original project work.
Further redistribution/reuse requires checking the relevant rights holders' terms.

## Icon Collections

The local HMI documentation identifies these collections as icon sources:

- [Google Material Design Icons](https://github.com/google/material-design-icons):
  upstream [Apache 2.0 license](https://github.com/google/material-design-icons/blob/master/LICENSE).
- [Pictogrammers MaterialDesign](https://github.com/Templarian/MaterialDesign):
  upstream [license](https://github.com/Templarian/MaterialDesign/blob/master/LICENSE)
  describes Apache 2.0 for icons, with possible respective licenses for
  redistributed icons. Its MIT code license must not be assumed to cover icons.

Local files include active/inactive color variants. Before distribution, match
each retained asset to its upstream source and revision, retain copyright and
modification notices, and include the applicable license texts. Collection-level
license checks do not certify every local SVG's provenance.

Official collection license texts are preserved unmodified in
[docs/licenses](docs/licenses/README.md).

## Vehicle And Legacy Artwork

`hmi/cluster_ui/assets/compact-city-ev.png` is the current static vehicle image.
Its embedded C2PA metadata identifies OpenAI Media Service API / gpt-image 2.0,
an algorithmically generated image, and a creation timestamp of 2026-08-17.
This metadata was inspected on 2026-10-08; the signature was not independently
validated. The image is recorded as generated visual artwork, not an original
vehicle photograph or an official manufacturer asset. No separate blanket
open-source license is assigned to it by this handover.

Other vehicle images, backgrounds and legacy SVGs also remain in the asset
directory. Their individual origins are not all verified. Being unused at
runtime does not remove redistribution considerations when included in a repository.

Do not publish unverified artwork as original work. Establish provenance or
replace/remove it with authorized assets in a separately reviewed change. The
approved UI has not been changed during this packaging audit.

## Runtime Dependencies

The project uses Qt, Arduino/ESP32 platform frameworks, libmosquitto and
256dpi/MQTT (the ESP32 `MQTTClient` library, not PubSubClient) through
the build toolchains. Preserve their upstream notices where required. Binary
distribution needs its own dependency-license review; a source-only release is
not evidence that a bundled application satisfies those obligations.

## Project License

The handover uses an explicit no-blanket-license policy in LICENSE.md rather than
applying MIT to mixed-provenance material. A root notice cannot override the
licenses or rights of third-party code/artwork. Provenance verification remains
a redistribution consideration, not a certified legal review.
