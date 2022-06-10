-- Mușuroi Daniel-Nicușor, 323CB --

                        Tema2 - PCom

    Intre server si clientii TCP se trimit mesaje de dimensiuni standard,
astfel, se evita situatia unor concatenari sau trunchieri de mesaje. 
Alte solutii pentru evitarea situatiilor de genul asta erau fie sa trimit, 
inaintea fiecarui mesaj, dimensiunea acestuia, fie sa am un delimitator
de mesaje. 
    Se folosesc structurile:
- UDP_message ce contine topicul, tipul informatiei si informatia.
- TCP_message ce contine un mesaj UDP(UDP_message), IP-ul clientului UDP
                ce transmite acest mesaj si portul.
- TCP_client ce contine socketul asociat acestui client, daca acesta este
                online sau nu, topicurile la care este abonat prin optiunea
                Store & Forward, numarul acestor topicuri, mesajele stocate
                si numarul mesajelor stocate.
    De asemenea, se retin asocieri intre ID-urile clientilor si clienti, 
intre topicuri si vectori de ID-uri de clienti (ce reprezinta clientii abonati
la un anumit topic) si intre socket-uri si ID-urile clientilor.

    SUBSCRIBER(CLIENT TCP):

    1. Se realizeaza conexiunea cu serverul si se trimite un mesaj cu
        ID-ul clientului curent.
    2. Se pot primi mesaje atat de la intrarea STDIN, cat si de la server.
    3. Daca se primeste mesaj de la STDIN, se verifica daca acesta
        este un mesaj de EXIT, SUBSCRIBE sau UNSUBSCRIBE.
        3.1. Daca este un mesaj de EXIT, se inchide socketul si se opreste
            programul.
        3.2. Daca este un mesaj de SUBSCRIBE, se trimite mesajul citit 
            de la tastatura la server.
        3.3. Daca este un mesaj de UNSUBSCRIBE, se trimite mesajul citit 
            de la tastatura la server.
    4. Daca se primeste un mesaj de la server, se analizeaza acest mesaj si 
        se afiseaza conform enuntului mesajul.

    SERVER:

    1. Se creeaza socketul UDP si cel TCP. Pe parcurs, se retin date despre
        clienti, topicuri si clientii abonati la aceste topicuri, socket-uri si
        clientii asociati acestor socket-uri.
    2. Se pot primi mesaje de la STDIN, client UDP, TCP (de conectare a unui
        nou client), client TCP.
    3. Daca se primeste un mesaj de la STDIN, se verifica daca acest mesaj 
        este de exit si in caz afirmativ, se inchid toate socket-urile deschise.
    4. Daca se primeste un mesaj de la un client UDP, se primeste mesajul UDP,
        se creeaza un mesaj TCP si se trimite acest mesaj la clienti abonati,
        care sunt online. Daca un client nu este online, dar acesta este abonat
        la topic prin optiunea Store & forward, atunci se stocheaza mesajul.
    5. Daca se primeste un mesaj de la TCP, inseamna ca apare o cerere de 
        conectare a unui client. Se da accept la aceasta conexiune, se primeste
        ID-ul clientului. Daca ID-ul clientului nu exista deja, atunci se
        creeaza un nou client si se afiseaza un mesaj corespunzator.
        Daca ID-ul clientului exista deja, se verifica daca 
        acesta este online sau nu. Daca este online atunci inseamna ca noul 
        client incearca sa se conecteaza cu un ID al altui client. Daca nu este
        online, atunci acest client incearca sa se reconecteaza. La reconectare,
        se trimit toate mesajele salvate cu optiunea Store & Forward.
    6. Daca se primeste un mesaj de la un client TCP, atunci se verifica daca
        acest mesaj este de SUBSCRIBE sau UNSUBSCRIBE.
        6.1. Daca este de SUBSCRIBE, atunci se verifica daca topicul la care 
            se da subscribe exista deja. Daca exista, se adauga acest client
            la vectorul de clienti ce au subscribe la topic. Daca nu exista, 
            atunci se creeaza un nou topic la care va fi abonat doar clientul
            curent momentan. Daca clientul se aboneaza la acest topic cu 
            optiunea store and forward atunci se retine acest lucru.
        6.2. Daca este de UNSUBSCRIBE, atunci se sterge clientul din vectorul
            de clienti de la acest topic si se sterge topicul din topicurile
            cu optiunea Store & Forward ale acestui client (daca este cazul).
