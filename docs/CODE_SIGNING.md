# Code signing policy

## Current status

No SignPath Foundation approval or production signing configuration is in place.
All current local/CI builds are unsigned. An artifact upload is not a signed release.

## Intended service

The project intends to apply to SignPath Foundation. If accepted, the signing
attribution will be: "Free code signing provided by SignPath.io, certificate by
SignPath Foundation". This is a planned attribution, not a current sponsorship claim.

## Roles

Proposed initial maintainer, reviewer and release approver:
[THREAD-the-toaster](https://github.com/THREAD-the-toaster).
Confirm these assignments during onboarding. Repository and SignPath members must
use MFA. Outside contributions, including workflow changes, require maintainer review.
Each production signing request requires an explicit authorized human approval.

## Release scope

Only MONOLITH executables built from reviewed project sources on GitHub-hosted
runners are eligible. Downloaded applications and third-party binaries must never
be submitted for signing as MONOLITH code. Check product name/version metadata
against the reviewed release version, preserve the source commit and build link,
and verify the returned Authenticode signature before publishing.

## Privacy

See [Privacy policy](../PRIVACY.md). The current desktop prototype performs no
network communication. Update this policy before adding network features.
