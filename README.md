A dokumentáció folyamatosan készül:
http://svn.kkfk.bgf.hu/lanview2.doc/lv2.html

Kisérleti jelleggel egyszerüsítem az öröklés fát.
A node host és snmpdevice objektumhoz közvetlenül hozzá van adva egy "main" port, és máshol is
van hasonló stípusu megoldás.
Miután hozzákezdtem a modosításhoz egyre nyilvánvalóbb, hogy a fenti egy nagyon rossz ötlet volt.
Tovább egyszerüsítettem azzal, hogy összevontam a cNode és cHost objektumot.
És teljesen ekvivalensé tettem az örökési láncot az adatbázisban és a c++ objektumokban, így az erre épülő
konverzios metódusok mindíg megfelelően müködnek.
