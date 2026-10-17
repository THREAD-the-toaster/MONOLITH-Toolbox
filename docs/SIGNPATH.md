# SignPath Foundation onboarding

This is a preparation checklist, not an approval or an active signing setup.

1. Publish the source repository publicly with LICENSE and accurate feature/limitation
   documentation. Include all source and scripts needed to build the released program.
2. Run the Windows workflow and review its portable package. Publish a clearly labelled
   unsigned prototype release, recording its source commit and known limitations.
3. Confirm maintainer/reviewer/approver roles, MFA and contribution review procedures.
4. Apply via https://signpath.org/apply. A new repository alone does not establish
   eligibility: the Foundation also evaluates release history, maintenance and project
   reputation. Acceptance is discretionary.
5. After acceptance, configure the SignPath project, artifact restrictions, trusted
   GitHub build integration and manual production approval with their guidance.
6. Add signing credentials through GitHub's secret UI, never source files or chat.
   Only then enable a signing workflow and verify the returned signature before release.

## Existing preparation

- MIT license, privacy and code signing policy documents.
- Windows version resource with matching product name and project version.
- GitHub-hosted MSVC build with an unsigned artifact and no signing credentials.
- Build/IDE files and private signing material excluded from source publication.

## Future GitHub integration

SignPath's documented flow uploads the unsigned artifact, then passes its artifact ID
to `signpath/github-action-submit-signing-request@v3`. The organization ID, project
slug, signing-policy slug and API token do not exist until onboarding is completed.
Require manual approval for production signing; do not expose signing secrets to
pull-request builds. Do not label local builds or CI artifacts as signed.

## References

- https://signpath.org/terms
- https://signpath.org/apply
- https://docs.signpath.io/trusted-build-systems/github
