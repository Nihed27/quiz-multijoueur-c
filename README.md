# 🎮 QUIZY — Quiz Multijoueur en Temps Réel

Projet de Programmation Systèmes et Réseaux — inspiré de Kahoot.

## 📁 Structure

    quiz_projet/
    ├── common/protocol.h
    ├── server/server.c
    ├── server/network.c/.h
    └── server/questions.txt

## ⚙️ Compilation

    cd server
    gcc -o server server.c network.c -lpthread

## 🚀 Exécution

    ./server
    ./client_quiz 192.168.X.X

## 🔧 Technologies
- Langage C (POSIX)
- Sockets TCP/IP (port 12345)
- Threads POSIX (pthread)
- Mutex & variables de condition
- Kali Linux
