#include <iostream>
#include <cstring>
#include <regex>
#include <curl/curl.h>
#include <gumbo.h>
#include <cstdio>
#include <cstdlib>

// Fonction de rappel pour écrire les données reçues dans une chaîne de caractères
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);
    return totalSize;
}

// Fonction pour extraire le texte d'un nœud
std::string ExtractText(GumboNode* node) {
    if (node->type == GUMBO_NODE_TEXT) {
        return std::string(node->v.text.text);
    } else if (node->type == GUMBO_NODE_ELEMENT || node->type == GUMBO_NODE_DOCUMENT) {
        std::string text = "";
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            std::string childText = ExtractText(static_cast<GumboNode*>(children->data[i]));
            text += childText;
        }
        return text;
    } else {
        return "";
    }
}

// Fonction pour extraire les titres à partir du contenu HTML
void ExtractTitles(GumboNode* node, const std::regex& regex) {
    if (node->type == GUMBO_NODE_ELEMENT) {
        if (node->v.element.tag == GUMBO_TAG_H1 ||
            node->v.element.tag == GUMBO_TAG_H2 ||
            node->v.element.tag == GUMBO_TAG_H3 ||
            node->v.element.tag == GUMBO_TAG_H4 ||
            node->v.element.tag == GUMBO_TAG_H5 ||
            node->v.element.tag == GUMBO_TAG_H6) {
            std::string text = ExtractText(node);
            if (std::regex_search(text, regex)) {
                std::cout << "Titre trouvé : " << text << std::endl;
            }
        }
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            ExtractTitles(static_cast<GumboNode*>(children->data[i]), regex);
        }
    }
}

// Fonction pour extraire les liens à partir du contenu HTML
void ExtractLinks(GumboNode* node);

// Fonction pour transformer le href en lien cliquable
std::string TransformHrefToClickableLink(const std::string& href, const std::string& text) {
    std::string link = "<a href=\"" + href + "\">" + text + "</a>";
    return link;
}

// Fonction pour extraire les titres et les liens à partir du contenu HTML
void ExtractData(GumboNode* node, const std::regex& regex) {
    if (node->type == GUMBO_NODE_ELEMENT) {
        if (node->v.element.tag == GUMBO_TAG_H1 ||
            node->v.element.tag == GUMBO_TAG_H2 ||
            node->v.element.tag == GUMBO_TAG_H3 ||
            node->v.element.tag == GUMBO_TAG_H4 ||
            node->v.element.tag == GUMBO_TAG_H5 ||
            node->v.element.tag == GUMBO_TAG_H6) {
            std::string text = ExtractText(node);
            if (std::regex_search(text, regex)) {
                std::cout << "Titre trouvé : " << text << std::endl;
            }
        } else if (node->v.element.tag == GUMBO_TAG_A) {
            GumboAttribute* href = gumbo_get_attribute(&node->v.element.attributes, "href");
            if (href) {
                std::string text = ExtractText(node);
                if (std::regex_search(text, regex)) {
                    std::cout << "Lien trouvé : " << TransformHrefToClickableLink(href->value, text) << std::endl;
                }
                // if (std::regex_search(text, regex)) {
                //     std::string sentiment = EvaluatePositivity(text);
                //     std::cout << "Texte trouvé : " << text << std::endl;
                //     std::cout << "Sentiment : " << sentiment << std::endl;
                // }
            }
        }
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            ExtractData(static_cast<GumboNode*>(children->data[i]), regex);
        }
    }
}

// Fonction pour effectuer une requête HTTP GET
std::string PerformHttpRequest(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Erreur lors de la récupération du contenu de la page : " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Impossible d'initialiser libcurl!" << std::endl;
    }
    return response;
}

// Fonction principale de scraping
void ScrapeWebsite(const std::string& url, const std::string& regexPattern) {
    std::regex regex(regexPattern);
    std::string response = PerformHttpRequest(url);
    if (!response.empty()) {
        GumboOutput* output = gumbo_parse(response.c_str());
        if (output) {
            // Appel de la fonction pour extraire les titres et les liens
            ExtractData(output->root, regex);
            gumbo_destroy_output(&kGumboDefaultOptions, output);
        }
    } else {
        std::cerr << "Impossible de récupérer le contenu de la page." << std::endl;
    }
}

int main() {
    // Appel de la fonction de scraping avec l'URL du site web cible et l'expression régulière
    ScrapeWebsite("https://www.allocine.fr/", "Gardiens*");
    std::cout << "scraping done." << std::endl;
    std::cout << "" << std::endl;

    const char* pythonCommand = "python3";
    const char* pythonScript = "sentiment_analysis.py";
    const char* text = "This movie is really great!";

    // Construire la commande pour exécuter le script Python avec le texte en tant qu'argument
    char command[256];
    snprintf(command, sizeof(command), "%s %s \"%s\"", pythonCommand, pythonScript, text);

    // Exécuter la commande
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        std::cerr << "Erreur lors de l'exécution du script Python." << std::endl;
        return EXIT_FAILURE;
    }

    // Lire la réponse du script Python depuis le pipe
    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    // Fermer le pipe
    pclose(pipe);

    // Utiliser le résultat obtenu depuis le script Python
    std::cout << "Sentiment : " << result;
    std::cout << "sentiment done." << std::endl;

    return EXIT_SUCCESS;
}
