FROM ubuntu:latest

# Atualize os repositórios e instale o sudo
RUN apt-get update && \
  apt-get install -y iputils-ping net-tools iperf && \
  apt-get install -y tcpdump && \
  apt-get install -y net-tools && \
  apt-get install -y sudo && \
  rm -rf /var/lib/apt/lists/*

# Adicione um usuário não-root (opcional)
RUN useradd -ms /bin/bash user

# Defina uma senha para o usuário (opcional)
RUN echo 'user:password' | chpasswd

# Adicione o usuário ao grupo sudo (opcional)
RUN usermod -aG sudo user
