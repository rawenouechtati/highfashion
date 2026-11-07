CREATE TABLE client (
    id        NUMBER PRIMARY KEY,
    nom       VARCHAR2(100) NOT NULL,
    email     VARCHAR2(100),
    telephone VARCHAR2(20),
    cin       VARCHAR2(8) UNIQUE NOT NULL,
    points    NUMBER DEFAULT 0,
    niveau    VARCHAR2(10) DEFAULT 'Bronze'
);

CREATE SEQUENCE client_seq START WITH 1 INCREMENT BY 1;

CREATE OR REPLACE TRIGGER client_trg
BEFORE INSERT ON client
FOR EACH ROW
WHEN (NEW.id IS NULL)
BEGIN
    SELECT client_seq.NEXTVAL INTO :NEW.id FROM dual;
END;
/

INSERT INTO client (nom, email, telephone, cin, points, niveau) VALUES ('Sarra Ben Ali',     'sarra.benali@mail.tn',  '22 345 678', '09128457', 540, 'Or');
INSERT INTO client (nom, email, telephone, cin, points, niveau) VALUES ('Yassine Trabelsi',  'y.trabelsi@mail.tn',    '98 120 647', '11234689', 320, 'Argent');
INSERT INTO client (nom, email, telephone, cin, points, niveau) VALUES ('Ines Mansouri',     'ines.mansouri@mail.tn', '55 908 231', '07468215',  85, 'Bronze');
INSERT INTO client (nom, email, telephone, cin, points, niveau) VALUES ('Karim Jebali',      'karim.jebali@mail.tn',  '29 774 610', '10983264', 510, 'Or');
INSERT INTO client (nom, email, telephone, cin, points, niveau) VALUES ('Nour Gharbi',       'nour.gharbi@mail.tn',   '50 231 889', '04871530', 210, 'Argent');
INSERT INTO client (nom, email, telephone, cin, points, niveau) VALUES ('Amine Chaouch',     'a.chaouch@mail.tn',     '23 456 019', '12056738',  45, 'Bronze');
INSERT INTO client (nom, email, telephone, cin, points, niveau) VALUES ('Rim Belhaj',        'rim.belhaj@mail.tn',    '97 665 302', '09345617', 455, 'Argent');
INSERT INTO client (nom, email, telephone, cin, points, niveau) VALUES ('Sami Dridi',        'sami.dridi@mail.tn',    '54 018 776', '11590824', 130, 'Bronze');
INSERT INTO client (nom, email, telephone, cin, points, niveau) VALUES ('Lina Hamdi',        'lina.hamdi@mail.tn',    '26 590 143', '08762094', 275, 'Argent');

COMMIT;