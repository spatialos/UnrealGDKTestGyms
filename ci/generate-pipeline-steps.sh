#!/bin/bash

DEBUG = "on"

set -e -u -o pipefail
if [[ -n "${DEBUG-}" ]]; then
    set -x
fi

# Detection code copied from ci folder from UnrealGDKExampleProject 
# Resolve the GDK branch to run against. The order of priority is:
# GDK_BRANCH envvar > same-name branch as the branch we are currently on > UnrealGDKVersion.txt > "master".
GDK_BRANCH_LOCAL="${GDK_BRANCH:-}"
if [ -z "${GDK_BRANCH_LOCAL}" ]; then
    GDK_REPO_HEADS=$(git ls-remote --heads "git@github.com:spatialos/UnrealGDK.git" "${BUILDKITE_BRANCH}")
    TESTGYMS_REPO_HEAD="refs/heads/${BUILDKITE_BRANCH}"
    if echo "${GDK_REPO_HEADS}" | grep -qF "${TESTGYMS_REPO_HEAD}"; then
        GDK_BRANCH_LOCAL="${BUILDKITE_BRANCH}"
    else
        GDK_VERSION=$(cat UnrealGDKVersion.txt)
        if [ -z "${GDK_VERSION}" ]; then
            GDK_BRANCH_LOCAL="master"
        else
            GDK_BRANCH_LOCAL="${GDK_VERSION}"
        fi
    fi
fi

sed "s|GDK_BRANCH_PLACEHOLDER|${GDK_BRANCH_LOCAL}|g" "ci/premerge.template.steps.yaml" | buildkite-agent pipeline upload
