//  The MIT License
//
//  Copyright (C) 2025 Giuseppe Mastrangelo
//
//  Permission is hereby granted, free of charge, to any person obtaining
//  a copy of this software and associated documentation files (the
//  'Software'), to deal in the Software without restriction, including
//  without limitation the rights to use, copy, modify, merge, publish,
//  distribute, sublicense, and/or sell copies of the Software, and to
//  permit persons to whom the Software is furnished to do so, subject to
//  the following conditions:
//
//  The above copyright notice and this permission notice shall be
//   included in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include <vector>
#include <string>
#include <random>
#include "PluginAPI.h"
#include "json.hpp"

std::vector<std::string> messages = {
            "Amor che nella mente mi ragiona cominciò egli a dir si dolcemente che la dolcezza ancor dentro mi suona. (Dante)",
            "Che cosa sarebbe l'umanità, signore, senza la donna? Sarebbe scarsa, signore, terribilmente scarsa. (Mark Twain)",
            "A chi più amiamo, meno dire sappiamo. (Proverbio inglese)",
            "A donna dona baci. (Anonimo)",
            "A provocare un sorriso è quasi sempre un altro sorriso. (Anonimo)",
            "Al cor gentil repara sempre Amore… (G. Guinizzelli)",
            "Allor fui preso, e non mi spiacque poi; sì dolce lume uscia dagli occhi suoi. (Petrarca)",
            "Ama e fai quel che vuoi. (S. Agostino)",
            "Amano davvero, quelli che tremano a dire che amano. (Ph. Sidney)",
            "Amare è gioire, mentre crediamo di gioire solo se siamo amati. (Aristotele)",
            "Amare è la metà di credere. (V.Hugo)",
            "Amare è mettere la nostra felicità nella felicità di un altro. (G.W.von Leibnitz)",
            "Amare è scegliere, baciare è la sigla della scelta. (Anonimo)",
            "Amare se stessi è l'inizio di un idillio che dura una vita. (Oscar Wilde)",
            "Amare significa agire ed esaltarsi senza tregua. (Emile Verhaeren)",
            "Amate, amate, tutto il resto è nulla. (La Fontaine)",
            "Amico mio, non pensiamo al domani e cogliamo insieme quest'attimo della vita che trascorre. (Kyyam)",
            "Amor s'io posso uscir dei tuoi artigli appena creder posso che alcun altro uncin mai + mi pigli. (Boccaccio)",
            "Amore è credula creatura. (Ovidio)",
            "Amore è rivelazione improvvisa: il bacio è sempre una scoperta. (Anonimo)",
            "Amore e tosse non si possono nascondere. (Ovidio)",
            "Amore guarda non con gli occhi ma con l'anima… (Shakespeare)",
            "Amore non è guardarsi a vicenda; è guardare insieme nella stessa direzione. (A.deSaint-Exupery)",
            "Amore! Ecco un volume in una parola, un oceano in una lacrima, un turbine in un sospiro, un millennio in un secondo. (Tupper)",
            "Amore, amabile follia… (Nicolas Chamfort)",
            "Amore, amore, che schiavitù l'amore. (La Fontaine)",
            "Amore, fuoco una volta mi trasse con un sol, lungo bacio tutta l'anima di tra le labbra, così come il sole beve la rugiada. (Tennyson)",
            "Amore, impossibile a definirsi! (Giacomo Casanova)",
            "Anima, Bacio, Cuore: l'ABC dell'amore. (Anonimo)",
            "Armoniosi accenti - dal tuo labbro volavano, e dagli occhi ridenti - traluceano di Venere i disdegni e le paci - la speme, il pianto e i baci. (Ugo Foscolo)",
            "Arricchiamoci delle nostre reciproche differenze. (Paul Valery)",
            "Baci ardenti come il sole, baci profondi come la notte. (Anonimo)",
            "Baci avuti facilmente si dimenticano facilmente. (Proverbio inglese)",
            "Baci uguali non esistono: ogni bacio ha un suo sapore. (Anonimo)",
            "Baci: parole che non si possono scrivere. (Anonimo)",
            "Bacia gli occhi della tua donna se sono velati di lacrime. (Anonimo)",
            "Baciami 100 e 1000 volte all'ora e +, se + baciarmi ancor tu puoi: pareggino le stelle i baci tuoi, ed il numero lor raddoppia ognora. (Orsetto Giustiniani)",
            "Baciamoci, Aminta mia, io bacio, se tu baci: bacia che bacio anch'io. (Cavalier Marino)",
            "Bacio. Primula nel giardino delle carezze. (P.Verlaine)",
            "Bacio: abbandono del cuore quando non è più solo. (Anonimo)",
            "Bacio: amore, fedeltà, amicizia, affetto, adorazione, dolcezza. (Anonimo)",
            "Bene tu fai, Amore, a celebrare la tua festa solenne nel virginal febbraio. (Patmore, S.Valentino)",
            "Bisogna scegliere tra amare le donne e conoscerle: non c'è via di mezzo. (Chamfort)",
            "Bocca dolcissima, se parli o taci sei tutta amori, sei tutta grazie e sempre affabili, sempre vivaci. (Rolli)",
            "Brinda a me solo con gli occhi...o solo lascia un bacio nella coppa, e non chiederò vino (B.Jonson)",
            "Celami in te, dove cose più dolci son celate, fra le radici delle rose e delle spezie. (Swinburne)",
            "Che cos'è il piacere, se non un dolore straordinariamente dolce? (Heinrich Heine)",
            "Che diano o che rifiutino, godono tuttavia d'esser richieste. (Ovidio, L'arte di amare)",
            "Che faccenda maledettamente pazza è l'amore. (Emanuel Schikaneder)",
            "Che l'amore è tutto, è tutto ciò che sappiamo dell'amore. (Emily Dickinson)",
            "Chi ama non teme la tempesta, teme solo che l'amore si spenga. (Anonimo)",
            "Chi ora fugge, presto inseguirà, chi non accetta doni, ne offrirà, e se non ama, presto comunque amerà. (Saffo)",
            "Chi vive amante sai che delira, spesso si lagna, sempre sospira né d'altro parla che di morir. (Metastasio)",
            "Chiunque ami... crede nell'impossibile. (Elizabeth Barret Browning)",
            "Ci si trova per caso, ci si incontra con un bacio. (Anonimo)",
            "Ci sono due cose che ho sempre amato alla follia: le donne e il celibato. (N.Chamfort)",
            "Ciò che diventa scherzo sulle labbra di tutti lo sentiamo più sacro, noi 2, nel nostro cuore. (Hulsoff)",
            "Ciò che una donna dice a un amante scrivilo nel vento, o nell'acqua che va rapida. (Catullo)",
            "Coloro che vivono d'amore vivone d'eterno. (Emile Verhaeren)",
            "Come in fondo al mare, ci sono sirene in fondo alle pupille. (Lorrain)",
            "Come sentiamo, così vogliamo essere sentiti. (H.von Hofmannsthal)",
            "Com'è soave l'amore quand'è sincero da entrambe le parti, è un airone bianco sulla neve: l'occhio non lo separa. (Anonimo giapponese)",
            "Come ti vidi mi innamorai. E tu sorridi perché lo sai. (Arrigo Boito)",
            "Con te conversando, dimentico ogni tempo e le stagioni e i loro mutamenti: tutte mi piacciono allo stesso modo. (Milton)",
            "Cos'è un bacio? Nulla. Cosa può essere? Tutto. (Anonimo)",
            "Cos'è un tuo bacio? Un lambire di fiamma… (VictorHugo)",
            "Cupido se ne va in giro a scagliare frecce: sfortunatamente però, non è mai stato a scuola di tiro con l'arco... (Anonimo)",
            "Da capo a piedi son fatta per l'amore; è tutto il mio mondo e altro non esiste. (dal'Angelo Azzurro)",
            "Da un uomo grande c'è qualcosa da imparare anche quando tace. (Seneca)",
            "Dal giorno che tu baci una donna, non sai più di che colore sono i suoi occhi. (Anonimo)",
            "Dall'amicizia all'amore c'è la distanza di un bacio. (Anonimo)",
            "Dammene uno dei tuoi più amorosi. Te ne renderò 4 più caldi che braci. (L.Labè)",
            "Dammi 100 baci, baci 1000 e ancora 100 baci baci, 1000 baci.… (Catullo)",
            "Dammi 1000 baci e quindi 100 e quindi altri 1000 ed altri 100 e poi di nuovo 1000 e ancora 100. (Catullo)",
            "Dell'amicizia a prima vista, come dell'amore a prima vista, va detto che è la sola vera. (H.Melville)",
            "Dice più un bacio che una dichiarazione d'amore. (Anonimo)",
            "Dolce Elena, fammi eterno con un bacio; suggono le sue labbra la mia anima: guarda ove vola. (Marlowe)",
            "Dolce, rossa, spendida bocca che bacia. (A.C.Swindburne)",
            "Donna, l'ora declina. Dammi la bocca porporina, ch'io la baci. (Walther von der Vogelweide)",
            "Dove gli occhi van volentieri, anche il cuore va, né il piede tarda a seguirli. (Carlo Dossi)",
            "Dovunque brilla amore, si riflette in un bacio. (Anonimo)",
            "E che cos'è un bacio? Un apostrofo rosa fra le parole t'amo, un segreto detto sulla bocca. (Rostand)",
            "E che cos'è un bacio? Un segreto detto sulla bocca, un istante di infinito che ha il fruscio di un'ape tra le piante. (Rostand)",
            "E' dolce quello che tu mi dici, ma più dolce è il bacio che ho rubato alla tua bocca. (H.Heine)",
            "E guardò gli occhi splendenti di luce, e le baciò le dolci labbra. Secondo voi non aveva ragione? (Anonimo)",
            "E' il ben pensare che conduce al ben dire. (Francesco De Sanctis)",
            "E lì le chiuse gli occhi / selvaggi e appassionati / con quattro baci. (J.Keats)",
            "E' più facile dare un bacio che dimenticarlo. (Anonimo)",
            "E' preferibile l'aver amato e aver perso l'amore al non aver amato affatto. (Lord Tennyson)",
            "E stupisco se non è questo, quello che si chiama Amore. (Ovidio)",
            "E' viva la tua anima? e lascia che si nutra! Non lasciare balconi da scalare, né bianchi seni su cui riposare, né teste d'oro con guanciali da spartire. (Edgar Lee Masters, Antologia di Spoon River)",
            "Ed io che intesi quel che non dicevi, m'innamorai di te perché tacevi. (Guerrini)",
            "Eravamo insieme, tutto il resto del tempo l'ho scordato. (Walt Whitman)",
            "Fa del tuo amore una pioggia di Baci sulle mie labbra. (Shelley)",
            "Finchè la vita, e la stagion d'amore/ ad amare ci invitano..Vieni amor mio, e sia colma la deliziosa messe dei baci. (de Ronsard)",
            "Fortunato quanto gli dei a me pare colui che siede di fronte a te e da vicino ode la tua voce e il riso melodioso. (Saffo)",
            "Fra due cuori che si amano non occorrono parole (Desbordes-Valmore)",
            "Fra i rumori della folla ce ne stiamo noi 2 felici di essere insieme, parlando poco, forse nemmeno una parola. (Whitman)",
            "Fu il tuo bacio, amore, a rendermi immortale. (M.Fuller)",
            "Giove, dall'alto, ride dei falsi giuramenti degli amanti. (Ovidio, L'Arte di Amare)",
            "Gli amici non sono né molti né pochi ma in numero sufficiente. (Hugo von Hofmannsthal)",
            "Gli uomini guardano le donne x vederle; le donne guardano gli uomini x essere viste. (Normand)",
            "Gli uomini sono sempre sinceri. Cambiano sincerità, ecco tutto. (Tristan Bernard)",
            "I baci mettono a nudo il cuore e vestono l'amore. (Anonimo)",
            "I baci sono le monete spicciole dell'amore. (Taddeus)",
            "I bisticci degli amanti rinnovano l'amore. (Terenzio, Andria)",
            "I gesti dicono poco; le parole, un po' di più; i baci, tutto. (Anonimo)",
            "I mariti delle donne che ci piacciono sono sempre degli imbecilli. (G.Feydeau)",
            "I ragazzi che si amano si baciano in piedi...nell'abbagliante chiarezza del loro 1°amore. (Prevert)",
            "Il bacio ama il blu: preferisce la notte. (Anonimo)",
            "Il bacio dovrebbe essere un'erba spontanea, non una pianta da giardino. (Anonimo)",
            "Il bacio è come la musica, il solo linguaggio universale. (Anonimo)",
            "Il bacio è la più alta poesia dell'amore. (Anonimo)",
            "Il bacio è un dolce trovarsi dopo essersi a lungo cercati. (Anonimo)",
            "Il bacio è una promessa scritta dalle labbra. (Anonimo)",
            "Il bacio fa più giovane il cuore e cancella le età. (Anonimo)",
            "Il colpo di fulmine è la cosa che fa guadagnare più tempo. (F.Arnoul)",
            "Il cuore di una donna è sfuggente come una goccia d'acqua su una foglia di loto. (Proverbio cinese)",
            "Il cuore ha le sue ragioni che la ragione non conosce. (Pascal, Pensieri)",
            "Il cuore non ha rughe. (M.mede Sevignè)",
            "Il cuore sente la testa confronta. (Chateaubriand)",
            "Il denaro è come le donne: per tenerlo, bisogna occuparsene. (F.Bourdet)",
            "Il flirt è l'acquerello dell'amore. (Bouerget)",
            "Il matrimonio è una catena così pesante che x portarla bisogna essere in 2: e spesso in 3. (Dumasfiglio)",
            "Il silenzio di un bacio vale più di mille parole. (Anonimo)",
            "Il silenzio più eloquente: quello di due bocche che si baciano. (Anonimo)",
            "Il sole abbraccia la Terra e i raggi di Luna baciano il mare. Ma a che vale tutto questo baciare se tu non baci me (Shelley)",
            "Il suono del bacio non è forte come quello di una cannonata, ma la sua eco dura molto più a lungo. (O.W.Holmes)",
            "Il tuo amore è x me come le stelle del mattino e della sera, tramonta dopo il sole e prima del sole risorge. (V.Goethe)",
            "Il vero amore è come l'apparizione degli spiriti: tutti ne parlano, quasi nessuno li ha visti. (F. de la Rochefoucauld)",
            "Il vero amore non ha mai conosciuto misura. (Properzio)",
            "In amore chi arde non ardisce e chi ardisce non arde. (Niccolò Tommaseo)",
            "In amore si scrive \"per sempre\", si legge \"fino a quando\". (Anonimo)",
            "Io capisco i tuoi baci, e tu i miei… (Shakespeare, EnricoIV)",
            "Io ti ringrazio, Amore, d'ogni pena e tormento, e son contento ormai d'ogni dolore. (Poliziano)",
            "La bellezza provoca i ladri più dell'oro. (Shakespeare, Comevigarba)",
            "la bocca mi baciò tutto tremante. Galeotto fu il libro e chi lo scrisse: quel giorno più non vi leggemmo avante. (Dante)",
            "La civetteria è lo champagne dell'amore. (T.Hood)",
            "La dieta dello scapolo: pane e formaggio e baci! (Proverbio inglese)",
            "La donna è come una buona tazza di caffè: la prima volta che se ne prende non lascia dormire. (A. Dumas padre) (A.Dumaspadre)",
            "La donna pensa come ama, l'uomo ama come pensa. (Mantegazza)",
            "La donna, nel paradiso terrestre, ha addentato il frutto proibito dieci minuti prima dell'uomo, e ha mantenuto poi sempre questi dieci minuti di vantaggio. (A.Karr)",
            "La felicità è nascosta dappertutto: basta scovarla. (Anonimo)",
            "La felicità in amore è come una palla che noi rincorriamo quando rotola, e che spingiamo via col piede quando si ferma. (M.mede Puissieux)",
            "La felicità risiede nel cuore e sulla bocca della donna amata. (Anonimo)",
            "La fortuna non è sempre e tutta opera del caso. (Baltasar Gracian)"
};

using json = nlohmann::json;

static PluginResource resources[] = {
        {
            "bacio-quote",
            "A list of the famous italian bacio perugina quotes",
            "bacio:///quote",
            "text/plain",
        }
};

const char* GetNameImpl() { return "bacio-quote"; }
const char* GetVersionImpl() { return "1.0.0"; }
PluginType GetTypeImpl() { return PLUGIN_TYPE_RESOURCES; }

int InitializeImpl() {
    return 1;
}

char* HandleRequestImpl(const char* req) {
    auto request = json::parse(req);
    nlohmann::json response = json::object();

    // Generate a random index to select a message
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, messages.size() - 1);

    // Build response
    nlohmann::json contents = json::array();
    contents.push_back(json::object({
        {"uri", resources[0].uri},
        {"typeType", resources[0].mime},
        {"text", messages[distr(gen)]}
    }));
    response["contents"] = contents;

    std::string result = response.dump();
    char* buffer = new char[result.length() + 1];
#ifdef _WIN32
    strcpy_s(buffer, result.length() + 1, result.c_str());
#else
    strcpy(buffer, result.c_str());
#endif

    return buffer;
}

void ShutdownImpl() {
}

int GetResourceCountImpl() {
    return sizeof(resources) / sizeof(resources[0]);
}

const PluginResource* GetResourceImpl(int index) {
    if (index < 0 || index >= GetResourceCountImpl()) return nullptr;
    return &resources[index];
}

static PluginAPI plugin = {
        GetNameImpl,
        GetVersionImpl,
        GetTypeImpl,
        InitializeImpl,
        HandleRequestImpl,
        ShutdownImpl,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        GetResourceCountImpl,
        GetResourceImpl
};

extern "C" PLUGIN_API PluginAPI* CreatePlugin() {
    return &plugin;
}

extern "C" PLUGIN_API void DestroyPlugin(PluginAPI*) {
    // Nothing to clean up for this example
}
