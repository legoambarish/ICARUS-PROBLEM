# OBC Round 2

This repository contains a standalone flight-software debugging exercise framed as an on-orbit operations build. AI coding tools are allowed and expected to be used as part of the workflow.

## Environment Setup

The exercise is distributed as a Docker image. You do not need to clone any repository — everything you need is inside the image. Follow the steps below to set up your working environment.

### Step A — Load the Image

After downloading the image file you were given, load it into Docker:

```bash
docker load -i icarus-round2.tar.gz
```

### Step B — Extract the Source onto Your Machine

This is a one-time step. It copies the project source out of the image and onto your local filesystem so you can edit it with your own tools:

```bash
docker create --name icarus-extract icarus-round2
docker cp icarus-extract:/obc ./icarus-workspace
docker rm icarus-extract
```

This creates a folder called `icarus-workspace` in your current directory containing the project source.

### Step C — Start a Working Session

Run the container with your extracted folder mounted so that edits persist on your machine:

```bash
docker run --rm -it -v <absolute-path-to-icarus-workspace>:/obc icarus-round2 bash
```

Replace `<absolute-path-to-icarus-workspace>` with the **absolute** path to the `icarus-workspace` folder you created in Step B. Examples for each operating system:

- **Linux / macOS:**
  ```bash
  docker run --rm -it -v /home/you/icarus-workspace:/obc icarus-round2 bash
  ```
- **Windows (native PowerShell):**
  ```powershell
  docker run --rm -it -v C:\Users\you\icarus-workspace:/obc icarus-round2 bash
  ```
- **Windows (Git Bash / WSL):**
  ```bash
  docker run --rm -it -v //c/Users/you/icarus-workspace:/obc icarus-round2 bash
  ```

This mounts your local folder into the container so any edits you make inside the container (or in your editor outside it) are immediately visible in both places, and all changes persist on your own filesystem when the container exits.

## Getting the Project Building

Once inside the container, your first task is to get the project to compile:

```bash
make
```

The build will produce the `obc_sim` binary.

## The Actual Task

Getting the project to build is only the first step. Once it compiles, run the simulation:

```bash
./obc_sim
```

A full run covers 2,250 ticks and takes approximately two minutes. As it runs, you will encounter behavior that does not match what a correctly functioning flight-software stack should do — crashes, incorrect readings, unexpected state changes, and other issues. Your task is to identify and fix these, using whatever combination of reading, running, testing, and AI-assisted investigation you find effective, so that the simulation behaves correctly across a full run.

You are not expected to find or fix everything. Work through as much as you can in the time available.

## Submitting Your Work

Submission is via two branches on a GitHub repository that you create yourself.

### Step E1 — Create a GitHub Repository

Create a new, empty repository on GitHub. This is your own repository — it is not provided to you.

### Step E2 — Initialize Git in Your Workspace

On your **host machine** (not inside the container), navigate to the `icarus-workspace` folder and initialize it as a Git repository:

```bash
cd icarus-workspace
git init
git remote add origin <your-repo-url>
```

### Step E3 — Push the "solution" Branch

Once your fixed simulation runs correctly end-to-end, commit this state and push it to a branch named exactly **`solution`**. This branch must contain your fully corrected code and nothing else added on top.

```bash
git checkout -b solution
git add .
git commit -m "Solution: all fixes applied"
git push -u origin solution
```

### Step E4 — Push the "fault-injection" Branch

Create a new branch from `solution` named exactly **`fault-injection`**. On this branch, add your fault-injection trigger (the `cmd_set_actuators` call or equivalent) and your filled-in `fault_manifest.json` with your predicted crash tick. Commit and push this branch separately.

```bash
git checkout -b fault-injection
# ... make your fault-injection changes and fill in fault_manifest.json ...
git add .
git commit -m "Fault injection: trigger and prediction"
git push -u origin fault-injection
```

### Step E5 — Final Checklist

- Both branches (`solution` and `fault-injection`) **must** exist on your GitHub repository at submission time.
- **Do not merge them together** — they are reviewed as two distinct, separate states.
- Include documentation explaining what you did, how you identified and fixed each issue, and if you were not able to fix certain things, what you would have done to fix them. Documentation holds significant value — avoid using AI tools for writing your documentation.

## Documentation Requirement

Your repository must include a written document (e.g. `DOCUMENTATION.md` at the repo root) covering every issue you identified and worked on. For each issue, explain:

- What the problem was and where you found it.
- Why it was actually a problem — the underlying mechanism, in your own words.
- How you fixed it (or, if you did not fix it, what approach you would have taken).

On the `fault-injection` branch, your documentation must also explain the physics and threshold reasoning behind your predicted crash tick — not just state a number.

**This documentation must be written by you.** AI tools may be used to investigate and fix code, but must not be used to write or heavily rephrase your documentation. It must demonstrate your own understanding of each issue. A correct fix with no explanation, or a shallow explanation that reads as generated or copied, scores lower than a correct fix with a clear, specific, evidently first-hand explanation.

There is no required format or length — just be clear and specific per issue.

## Notes

The exercise is intentionally scoped so that full completion is not expected within 48 hours. Partial, correct progress is still useful and is scored accordingly.