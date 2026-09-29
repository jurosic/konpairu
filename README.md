# konpairu
> *"finally a good compiler"*

## How to Contribute
To maintain a stable codebase, all changes to the `main` branch must be made through a pull/merge request. Pushes to `main` are restricted. 

### 1. Create a Feature Branch
Create a temporary local branch called a '*feature branch*' for your specific feature or bug fix. The name of the branch should reflect the changes made, e.g. `feat/lexer` or `fix/lexer-leak`. 
```bash
# Create a new branch
git checkout -b <branch-name-describing-changes>

# Verify you are on the right branch
git branch
```

### 2. Stage and Commit Your Work
Commit your work in small, logical chunks.
```bash
# Stage files
git add <files-with-changes>

# Commit with a clear but descriptive message
git commit -m "Brief description of the changes"
```

### 3. Keep Your Branch Updated (Crucial)
**Always sync your branch with `main` before pushing.** This prevents you from accidentally overwriting others' work and resolves merge conflicts locally before they reach GitHub.

```bash
# Fetch the latest history from the remote
git fetch origin

# Merge the latest main into your working tree
git merge origin/main
```
> **Note on Rebasing:** Merging (`git merge`) creates a merge commit, which preserves an accurate history of when branches diverged and reconnected. If you prefer a linear history, you can use `git rebase origin/main` instead. However, maintaining a clear history of changes is just soo recommended!!!!

### 4. Push and Open a Pull Request
Once you deem your feature complete and your branch is properly synced with `main`, you can push it to remote.
```bash
git push -u origin <branch-name-describing-changes>
```

**Next Steps on GitHub:**
1. Switch to your branch. 
    *(image will be added in next merge)* ![Branch selection dropdown](./docs/assets/branch-selection.png)
2. Click **Compare & pull request**.
    *(image will be added in next merge)* ![Pull request button](./docs/assets/pr-button.png)
3. Fill out the description with a summary of your changes.

Opening the request will automatically notify the team via WhatsApp and email. Once somebody on the team reviews your code (please review thoroughly), it can be merged to main, deleting the temporary branch in the process.

### 4.1 Backing up code
If you deem it necessary, feel free to also push code to GitHub without requesting a merge to back up your code on remote.

### 5. Clean Up After Merging
Once your PR is approved and merged into `main`, you can delete the branch locally and pull the newly updated `main` to your machine.
```bash
# Switch back to the main branch
git checkout main

# Pull the freshly merged code to your local machine
git pull origin main

# Delete your local feature branch
git branch -d <branch-name-describing-changes>
```
