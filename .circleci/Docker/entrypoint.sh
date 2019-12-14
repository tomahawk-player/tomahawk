#!/bin/bash

# Add local user
# Either use the LOCAL_USER_ID if passed in at runtime or
# fallback
USER_ID=${LOCAL_USER_ID:-9001}
USER=docker
UPWD=Docker!

echo "Starting with USER: $USER and UID : $USER_ID"
useradd --shell /bin/bash -u $USER_ID -o -c "docker user" -m "$USER"
export HOME=/home/$USER

# Add user to sudoers
echo "docker ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/10-installer
# Add root password
echo "root":$UPWD | chpasswd
# Add user password
echo "$USER:$UPWD" | chpasswd

if [ -z "$CLANG_VERSION" ]; then
  echo "No CLANG_VERSION set"
else
  echo "alias clang=clang-$CLANG_VERSION" >> $HOME/.bashrc
  echo "alias clang-tidy=clang-tidy-$CLANG_VERSION" >> $HOME/.bashrc
  echo "alias clang-format=clang-format-$CLANG_VERSION" >> $HOME/.bashrc
fi

# Startup user
exec gosu "$USER" "$@"
