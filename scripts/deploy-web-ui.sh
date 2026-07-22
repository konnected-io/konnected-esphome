#!/usr/bin/env bash
#
# Package and publish a device web UI (www.js / www.css) to the Konnected CDN.
#
# Layout on S3 (bucket: app.konnected.io, served via CloudFront at
# https://app.konnected.io) is scoped by platform and product model:
#
#   /esp/<product>/www.js          <- rolling "latest", short TTL, invalidated on deploy
#   /esp/<product>/www.css
#   /esp/<product>/<version>/www.js    <- immutable snapshot, 1-year TTL
#   /esp/<product>/<version>/www.css
#
# Firmware points at the rolling path so UI fixes ship without reflashing;
# pin a build to a /<version>/ path when you need the UI frozen with the firmware.
#
# Usage:
#   scripts/deploy-web-ui.sh [options]
#
#   --product NAME     product/model slug (default: gdo-blaq); source is web/NAME/
#   --version VER      immutable snapshot dir (default: v1)
#   --dry-run          show what would be uploaded, change nothing
#   --force            overwrite an existing snapshot whose content differs
#   --no-invalidate    skip the CloudFront invalidation
#   --yes, -y          don't prompt for confirmation
#   --help, -h         this message
#
set -euo pipefail

BUCKET="app.konnected.io"
CDN_HOST="app.konnected.io"
CDN_ORIGIN="https://$CDN_HOST"
PREFIX="esp"
PRODUCT="gdo-blaq"
VERSION="v1"
DRY_RUN=false
FORCE=false
INVALIDATE=true
ASSUME_YES=false

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

die() { printf '\033[31merror:\033[0m %s\n' "$*" >&2; exit 1; }
info() { printf '\033[36m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[33mwarn:\033[0m %s\n' "$*" >&2; }

usage() { sed -n '2,26p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0; }

while [[ $# -gt 0 ]]; do
  case "$1" in
    --product) PRODUCT="${2:?--product needs a value}"; shift 2 ;;
    --version) VERSION="${2:?--version needs a value}"; shift 2 ;;
    --dry-run) DRY_RUN=true; shift ;;
    --force) FORCE=true; shift ;;
    --no-invalidate) INVALIDATE=false; shift ;;
    -y|--yes) ASSUME_YES=true; shift ;;
    -h|--help) usage ;;
    *) die "unknown argument: $1 (try --help)" ;;
  esac
done

command -v aws >/dev/null || die "aws CLI not found"
command -v shasum >/dev/null || die "shasum not found"

SRC_DIR="$REPO_ROOT/web/$PRODUCT"
[[ -d "$SRC_DIR" ]] || die "no source directory at web/$PRODUCT"

ASSETS=(www.js www.css)
content_type_for() {
  case "$1" in
    *.js)  echo "application/javascript; charset=utf-8" ;;
    *.css) echo "text/css; charset=utf-8" ;;
    *)     echo "application/octet-stream" ;;
  esac
}

sha256_file() { shasum -a 256 "$1" | awk '{print $1}'; }

# --- validate sources -------------------------------------------------------

for asset in "${ASSETS[@]}"; do
  f="$SRC_DIR/$asset"
  [[ -f "$f" ]] || die "missing $f"
  [[ -s "$f" ]] || die "$f is empty"
done

# The logo is inlined into www.js at author time (see CLAUDE.md). If it is not
# in there at all, www.js is almost certainly stale or truncated.
if [[ -f "$SRC_DIR/logo-icon.svg" ]] && ! grep -q '<svg' "$SRC_DIR/www.js"; then
  warn "no inline <svg> found in www.js — is it in sync with logo-icon.svg?"
fi

ROLLING_BASE="$CDN_ORIGIN/$PREFIX/$PRODUCT/"
VERSIONED_BASE="$CDN_ORIGIN/$PREFIX/$PRODUCT/$VERSION/"

info "Product:   $PRODUCT   (source: web/$PRODUCT/)"
info "Bucket:    s3://$BUCKET/$PREFIX/$PRODUCT/"
info "Rolling:   $ROLLING_BASE"
info "Snapshot:  $VERSIONED_BASE"
echo

# --- compare against what is already live ----------------------------------

printf '%-10s %10s  %-16s %s\n' "ASSET" "BYTES" "SHA256 (local)" "STATUS"
SNAPSHOT_CONFLICT=false
ANY_CHANGE=false

for asset in "${ASSETS[@]}"; do
  f="$SRC_DIR/$asset"
  local_sha="$(sha256_file "$f")"
  bytes="$(wc -c < "$f" | tr -d ' ')"

  rolling_key="$PREFIX/$PRODUCT/$asset"
  version_key="$PREFIX/$PRODUCT/$VERSION/$asset"

  rolling_sha="$(aws s3api head-object --bucket "$BUCKET" --key "$rolling_key" \
                   --query 'Metadata.sha256' --output text 2>/dev/null || echo "")"
  version_sha="$(aws s3api head-object --bucket "$BUCKET" --key "$version_key" \
                   --query 'Metadata.sha256' --output text 2>/dev/null || echo "")"

  status=""
  if [[ -z "$rolling_sha" || "$rolling_sha" == "None" ]]; then
    status="new"; ANY_CHANGE=true
  elif [[ "$rolling_sha" == "$local_sha" ]]; then
    status="unchanged"
  else
    status="CHANGED"; ANY_CHANGE=true
  fi

  if [[ -n "$version_sha" && "$version_sha" != "None" && "$version_sha" != "$local_sha" ]]; then
    status="$status; snapshot $VERSION differs!"
    SNAPSHOT_CONFLICT=true
  fi

  printf '%-10s %10s  %-16s %s\n' "$asset" "$bytes" "${local_sha:0:16}" "$status"
done
echo

if [[ "$SNAPSHOT_CONFLICT" == true && "$FORCE" != true ]]; then
  die "snapshot $VERSION already exists with different content.
       Bump --version to keep snapshots immutable, or pass --force to overwrite it."
fi

if [[ "$ANY_CHANGE" != true ]]; then
  info "Rolling assets already match local files."
fi

if [[ "$DRY_RUN" == true ]]; then
  info "--dry-run: nothing uploaded."
  exit 0
fi

if [[ "$ASSUME_YES" != true ]]; then
  printf 'Publish to %s (public CDN)? [y/N] ' "$CDN_ORIGIN"
  read -r reply
  [[ "$reply" =~ ^[Yy]$ ]] || die "aborted"
  echo
fi

# --- upload -----------------------------------------------------------------

upload() { # <file> <key> <cache-control>
  local file="$1" key="$2" cc="$3"
  aws s3api put-object \
    --bucket "$BUCKET" \
    --key "$key" \
    --body "$file" \
    --content-type "$(content_type_for "$file")" \
    --cache-control "$cc" \
    --metadata "sha256=$(sha256_file "$file")" \
    --output text --query 'ETag' >/dev/null
  printf '    s3://%s/%s\n' "$BUCKET" "$key"
}

info "Uploading immutable snapshot ($VERSION)…"
for asset in "${ASSETS[@]}"; do
  upload "$SRC_DIR/$asset" "$PREFIX/$PRODUCT/$VERSION/$asset" \
    "public, max-age=31536000, immutable"
done

info "Uploading rolling latest…"
for asset in "${ASSETS[@]}"; do
  upload "$SRC_DIR/$asset" "$PREFIX/$PRODUCT/$asset" \
    "public, max-age=300"
done
echo

# --- invalidate -------------------------------------------------------------

if [[ "$INVALIDATE" == true ]]; then
  dist_id="${CDN_DISTRIBUTION_ID:-}"
  if [[ -z "$dist_id" ]]; then
    dist_id="$(aws cloudfront list-distributions \
      --query "DistributionList.Items[?contains(Aliases.Items || \`[]\`, '$CDN_HOST')].Id | [0]" \
      --output text 2>/dev/null || echo "None")"
  fi
  if [[ -z "$dist_id" || "$dist_id" == "None" ]]; then
    warn "could not resolve CloudFront distribution; set CDN_DISTRIBUTION_ID to invalidate."
  else
    info "Invalidating /$PREFIX/$PRODUCT/* on ${dist_id}…"
    inval_id="$(aws cloudfront create-invalidation \
      --distribution-id "$dist_id" \
      --paths "/$PREFIX/$PRODUCT/*" \
      --query 'Invalidation.Id' --output text)"
    printf '    invalidation %s (propagates in ~1 min)\n' "$inval_id"
  fi
  echo
fi

# --- verify -----------------------------------------------------------------

info "Verifying snapshot URLs…"
verify_failed=false
for asset in "${ASSETS[@]}"; do
  url="$VERSIONED_BASE$asset"
  remote_sha="$(curl -fsS "$url" 2>/dev/null | shasum -a 256 | awk '{print $1}' || echo "")"
  if [[ "$remote_sha" == "$(sha256_file "$SRC_DIR/$asset")" ]]; then
    printf '    \033[32mok\033[0m   %s\n' "$url"
  else
    printf '    \033[31mFAIL\033[0m %s\n' "$url"
    verify_failed=true
  fi
done
echo

if [[ "$verify_failed" == true ]]; then
  die "verification failed — deployed content does not match local files"
fi

info "Done. Firmware web_ui_base:"
printf '      %s\n' "$ROLLING_BASE"
printf '      %s   (pinned)\n' "$VERSIONED_BASE"
