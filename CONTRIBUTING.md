# Contributing to aio_uring
Welcome! And thank you a lot for your interest in contributing to aio_uring! \
Main goal of project is to bring production ready library for interaction with io_uring in Python, with support of NoGIL activity in Python.

**Special thanks for saving personal time of creating contribution docs to https://github.com/diggsweden/open-source-project-template**

## Getting in touch
Join the [Discord chat](https:/discord.gg/Z3ySMqt4R) and lets create this community together!

-----

## Ways to Contribute

As a new contributor, you're in an excellent position to provide valuable feedback. Here are some ways you can help:

- Fix or report a bug.
- Suggest enhancements to code, tests, and documentation.
- Help to adapt development process for different linux distros family
- Report or fix problems found during installation or in developer environments.
- Propose new features or improvements.

## Community Guideline

Be nice and respectful to each other.

We follow the [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md).

## File an Issue

Before creating a new issue, check if a similar one already exists.

If it does, add your information as a comment to the existing issue.

### Report a Bug

Reporting bugs is a valuable way to contribute:

1. Open an issue that summarizes the bug.
2. Set the label to "bug".

### Suggest a Feature

To request a new feature:

1. Open an issue summarizing the desired functionality and its use case.
2. Set the label to "enhancement" or "feature".

## Contribute Code, Documentation, and More

To contribute code, documentation, or other improvements:

1. Discuss your plans beforehand to ensure alignment with project goals.
2. Check the list of open issues. Assign yourself an existing issue or create a new one.
3. Follow project conventions for tests, code style, documentation, and commit messages.
4. Understand that contributions may be declined if they don't align with project guidelines or goals.
5. Familiarize yourself with the [Pull Request Lifecycle](#pull-request-lifecycle).
6. Agree to the "inbound=outbound" norm: your contributions will be under the same license as the project.
7. [Sign your commits](#dco---signoff-and-signing-a-commit).

## Issues and Pull Request Feedback

Project maintainers aim to review and respond to issues within 5 business days.

The quality of information in your issue or pull request affects the speed of feedback.

For non-trivial contributions, discuss with the project team first.

**If the project is not listed as archived, it is maintained.**

## Pull Request Lifecycle

We use the [Fork-and-Pull Model](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/getting-started/about-collaborative-development-models#fork-and-pull-model):

1. Fork the repository.
2. Create a topic branch from your fork's main branch.
3. Make your changes and run quality checks locally (if applicable).
4. Push your changes to the topic branch in your fork.
5. Open a new pull request to the main project.
6. Respond to any feedback from project maintainers.

## Commit Guidelines

### DCO - Signoff and Signing a Commit

#### Signoff (DCO agree)

***A Signoff assures the project that you have the right to contribute your content***

Add a signoff to your commit using the `-s` or `--signoff` flag:

```console
git commit --signoff -m 'fix: add fix for superbug x'
```

#### Sign

***A Sign assures that the commit came from you***

Sign your commit with `-S` or `--gpg-sign`:

```shell
git commit --signoff --gpg-sign -m "fix: add fix for the bug"
```

### Commit Standard

- Use the [Conventional Commit standard](https://www.conventionalcommits.org).
- Group relevant changes in commits.
- Write clear, human-readable commit messages.

## Reporting Security Issues

For security vulnerabilities, follow the guidelines in our [Security information](SECURITY.md).

## Development Guidelines

For development guidance, see the [DEVELOPMENT Guide](docs/DEVELOPMENT.md).

## Writing Style and Translations

- Keep documentation easy to read.
- Use bullet points for clarity.
- Be concise and link to external resources when needed.
- Use British English (e.g., "colour" instead of "color").
- Follow the [one-sentence-per-line](https://sembr.org/) principle in Markdown or AsciiDoc.

English is the primary language, with translations on a best-effort basis.

## FOSS Standards

We follow these standards and best practices:

- [REUSE License specification](https://reuse.software/)
    - Ensures clear and standardized license compliance across the project.

- [Conventional Commits format](https://www.conventionalcommits.org/en/v1.1.0/)
    - Provides a clear and structured project history through standardized commit messages.

- [Keep-A-Changelog format](https://keepachangelog.com/en/1.1.0/)
    - Maintains a clear and user-friendly release history.

- [Semantic Versioning 2.0.0](https://semver.org/)
    - Provides consistent version numbering for releases.

- [Contributor Covenant guidelines](https://www.contributor-covenant.org/)
    - Establishes a social contract for respectful and inclusive collaboration.

- [OpenSSF Scorecard](https://scorecard.dev/)
    - Helps assess and improve the security health of our project.

- [PublicCode.yml](https://yml.publiccode.tools/index.html)
    - Facilitates easy metadata indexing for better discoverability of our project.

- [Standard for Public Code](https://standard.publiccode.net/)
    - Ensures our project meets criteria for public code quality and sustainability.

Please familiarize yourself with these guidelines and help us maintain these standards in your contributions.

***Happy contributing!***
