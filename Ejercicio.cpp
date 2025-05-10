#include <iostream>
#include <vector>
#include <string>
#include <algorithm> // Para std::find_if, std::fill (aunque std::string lo hace internamente)
#include <numeric>   // Para std::accumulate
#include <limits>    // Para std::numeric_limits

// Es una buena práctica evitar 'using namespace std;' en el ámbito global,
// especialmente en archivos de cabecera. Para archivos .cpp pequeños como este,
// a veces se tolera, pero es mejor ser explícito o usarlo en ámbitos más reducidos.
// Por ahora, lo mantendremos para que el código sea similar al original,
// pero tenlo en cuenta para proyectos más grandes.
using namespace std;

// Nombre constante para los bloques libres
const string LIBRE_BLOCK_NAME = "Libre";
const int DEFAULT_TOTAL_MEMORY = 64; // MB
const int DISPLAY_BLOCKS_PER_LINE = 16; // Para la visualización

struct Bloque {
    string nombre;  // LIBRE_BLOCK_NAME o nombre del proceso
    int inicio;
    int tamaño;
};

class Memoria {
    vector<Bloque> bloques;
    int total_memoria;

public:
    Memoria(int size) : total_memoria(size) {
        if (size <= 0) {
            // Manejar tamaño inválido, por ejemplo, lanzar una excepción o usar un valor por defecto.
            // Aquí usaremos un valor por defecto y mostraremos una advertencia.
            cerr << "Advertencia: Tamaño de memoria inválido (" << size << "). Usando "
                 << DEFAULT_TOTAL_MEMORY << " MB en su lugar." << endl;
            total_memoria = DEFAULT_TOTAL_MEMORY;
        }
        bloques.push_back({LIBRE_BLOCK_NAME, 0, total_memoria});
    }

    // Método const porque no modifica el estado de la memoria
    void mostrar() const {
        cout << "\nEstado de la memoria (barra de " << total_memoria << " MB):\n";
        
        // Usar std::string para la vista es más seguro que un array C-style
        // y se inicializa directamente con puntos.
        string vista(total_memoria, '.');

        for (const auto& b : bloques) {
            if (b.nombre != LIBRE_BLOCK_NAME) {
                for (int i = 0; i < b.tamaño; ++i) {
                    int pos_en_vista = b.inicio + i;
                    if (pos_en_vista < total_memoria) { // Doble chequeo de seguridad
                        vista[pos_en_vista] = b.nombre[0]; // Primera letra del proceso
                    }
                }
            }
        }

        for (int i = 0; i < total_memoria; ++i) {
            cout << "|" << vista[i];
            if ((i + 1) % DISPLAY_BLOCKS_PER_LINE == 0 && i + 1 < total_memoria) {
                cout << "|\n"; // Salto de línea cada DISPLAY_BLOCKS_PER_LINE MB
            }
        }
        cout << "|\n"; // Cierre de la última línea
    }

    void cargarProceso(const string& nombre, int tamaño) {
        if (tamaño <= 0) {
            cout << "Error: Tamaño de proceso inválido (" << tamaño << " MB).\n";
            return;
        }
        if (nombre == LIBRE_BLOCK_NAME) {
            cout << "Error: El nombre del proceso no puede ser '" << LIBRE_BLOCK_NAME << "'.\n";
            return;
        }

        // Buscar un bloque libre que sea lo suficientemente grande (First-Fit)
        auto it = find_if(bloques.begin(), bloques.end(),
            [tamaño](const Bloque& b) {
                return b.nombre == LIBRE_BLOCK_NAME && b.tamaño >= tamaño;
            });

        if (it != bloques.end()) {
            if (it->tamaño == tamaño) {
                // El bloque libre es del tamaño exacto
                it->nombre = nombre;
            } else {
                // El bloque libre es más grande, hay que dividirlo
                Bloque nuevo_proceso = {nombre, it->inicio, tamaño};
                
                // Actualizar el bloque libre restante
                it->inicio += tamaño;
                it->tamaño -= tamaño;
                
                // Insertar el nuevo bloque de proceso antes del bloque libre modificado
                bloques.insert(it, nuevo_proceso);
            }
            cout << "Proceso '" << nombre << "' (" << tamaño << " MB) cargado.\n";
        } else {
            cout << "No hay suficiente espacio libre contiguo para el proceso '"
                 << nombre << "' (" << tamaño << " MB). Intenta compactar.\n";
        }
        mostrar(); // Mostrar estado después de la operación
    }

    void liberarProceso(const string& nombre) {
        if (nombre == LIBRE_BLOCK_NAME) {
            cout << "Error: No se puede liberar un bloque '" << LIBRE_BLOCK_NAME << "'.\n";
            return;
        }

        auto it = find_if(bloques.begin(), bloques.end(),
            [&nombre](const Bloque& b) {
                return b.nombre == nombre;
            });

        if (it != bloques.end()) {
            it->nombre = LIBRE_BLOCK_NAME;
            cout << "Proceso '" << nombre << "' liberado.\n";
            compactarEspaciosLibresAdyacentes(); // Fusionar bloques libres adyacentes
        } else {
            cout << "Proceso '" << nombre << "' no encontrado.\n";
        }
        mostrar(); // Mostrar estado después de la operación
    }

    void compactarMemoriaFisica() {
        vector<Bloque> bloques_compactados;
        int puntero_actual = 0;
        int espacio_libre_total = 0;

        // Mover todos los procesos al inicio de la memoria
        for (const auto& b : bloques) {
            if (b.nombre != LIBRE_BLOCK_NAME) {
                bloques_compactados.push_back({b.nombre, puntero_actual, b.tamaño});
                puntero_actual += b.tamaño;
            } else {
                espacio_libre_total += b.tamaño;
            }
        }

        // Añadir un único bloque libre al final con todo el espacio restante
        if (puntero_actual < total_memoria) { // O if (espacio_libre_total > 0)
             // Asegurarse de que el espacio libre calculado coincida con el restante
            if (total_memoria - puntero_actual != espacio_libre_total && !bloques_compactados.empty()) {
                 // Esto podría indicar un error en la lógica si no son iguales
                 // (asumiendo que compactarEspaciosLibresAdyacentes se llamó antes o los bloques libres estaban dispersos)
            }
            bloques_compactados.push_back({LIBRE_BLOCK_NAME, puntero_actual, total_memoria - puntero_actual});
        } else if (bloques_compactados.empty() && total_memoria > 0) {
            // Si toda la memoria estaba libre y se compactó, debería quedar un solo bloque libre
            bloques_compactados.push_back({LIBRE_BLOCK_NAME, 0, total_memoria});
        }


        bloques = bloques_compactados;
        cout << "Memoria compactada físicamente.\n";
        mostrar();
    }

    // Método const porque no modifica el estado de la memoria
    void calcularFragmentacionInterna() const {
        // La fragmentación interna real en este modelo es CERO, porque cada proceso
        // ocupa exactamente el tamaño que solicitó.
        // Lo que el código original simulaba podría ser más bien "desperdicio por asignación"
        // si se asignaran bloques de tamaño fijo más grandes que lo solicitado.
        // Mantenemos la lógica original con una aclaración.
        int frag_simulada = 0;
        for (const auto& b : bloques) {
            if (b.nombre != LIBRE_BLOCK_NAME && b.tamaño > 5) { // Umbral arbitrario
                frag_simulada += 1; // Simulación simple: 1MB por bloque "grande"
            }
        }
        cout << "Fragmentación interna (simulación simple de desperdicio por asignación): "
             << frag_simulada << " MB\n";
        cout << "(Nota: En este modelo de asignación exacta, la fragmentación interna real es 0 MB)\n";
    }

    // Método const porque no modifica el estado de la memoria
    void calcularFragmentacionExterna() const {
        // Suma de todos los bloques libres
        int frag_externa = accumulate(bloques.begin(), bloques.end(), 0,
            [](int sum, const Bloque& b) {
                return b.nombre == LIBRE_BLOCK_NAME ? sum + b.tamaño : sum;
            });
        
        // También se puede contar cuántos bloques libres hay, si son muchos pequeños, es peor.
        long num_bloques_libres = count_if(bloques.begin(), bloques.end(),
            [](const Bloque& b){ return b.nombre == LIBRE_BLOCK_NAME && b.tamaño > 0; });


        cout << "Fragmentación externa total: " << frag_externa << " MB";
        if (frag_externa > 0) {
            cout << " (en " << num_bloques_libres << " bloque(s) libre(s))";
        }
        cout << "\n";
    }

private:
    // Fusiona bloques libres que son adyacentes
    void compactarEspaciosLibresAdyacentes() {
        if (bloques.empty()) return;

        vector<Bloque> bloques_fusionados;
        bloques_fusionados.push_back(bloques[0]);

        for (size_t i = 1; i < bloques.size(); ++i) {
            Bloque& ultimo_fusionado = bloques_fusionados.back();
            const Bloque& actual = bloques[i];

            if (ultimo_fusionado.nombre == LIBRE_BLOCK_NAME && actual.nombre == LIBRE_BLOCK_NAME) {
                // Si el último bloque fusionado y el actual son libres, fusionarlos
                ultimo_fusionado.tamaño += actual.tamaño;
            } else {
                // Si no, simplemente añadir el bloque actual
                bloques_fusionados.push_back(actual);
            }
        }
        bloques = bloques_fusionados;
    }
};

// Función auxiliar para limpiar el buffer de entrada y manejar errores
void limpiarCin() {
    cin.clear(); // Limpiar los flags de error de cin
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Descartar la entrada incorrecta
}

int main() {
    int tam_memoria_inicial;
    cout << "Ingrese el tamaño total de la memoria (MB, ej. 64): ";
    cin >> tam_memoria_inicial;
    while(cin.fail() || tam_memoria_inicial <= 0) {
        cout << "Entrada inválida. Por favor, ingrese un número entero positivo para el tamaño: ";
        limpiarCin();
        cin >> tam_memoria_inicial;
    }


    Memoria mem(tam_memoria_inicial);
    int opcion;
    string nombre_proceso;
    int tamaño_proceso;

    do {
        cout << "\n--- SIMULADOR DE GESTIÓN DE MEMORIA ---\n";
        cout << "1. Cargar proceso (First-Fit)\n";
        cout << "2. Liberar proceso\n";
        cout << "3. Compactar memoria (físicamente)\n";
        cout << "4. Ver estado de la memoria\n";
        cout << "5. Calcular fragmentación interna (simulada)\n";
        cout << "6. Calcular fragmentación externa\n";
        cout << "0. Salir\n";
        cout << "Selecciona una opción: ";
        
        cin >> opcion;
        if(cin.fail()) {
            cout << "Opción no válida. Por favor, ingrese un número.\n";
            limpiarCin();
            opcion = -1; // Para que no coincida con ninguna opción válida y repita el bucle
            continue;
        }

        switch (opcion) {
            case 1:
                cout << "Nombre del proceso: ";
                cin >> nombre_proceso;
                cout << "Tamaño del proceso (MB): ";
                cin >> tamaño_proceso;
                if(cin.fail() || tamaño_proceso <=0) {
                    cout << "Tamaño de proceso inválido.\n";
                    limpiarCin();
                } else {
                    mem.cargarProceso(nombre_proceso, tamaño_proceso);
                }
                break;
            case 2:
                cout << "Nombre del proceso a liberar: ";
                cin >> nombre_proceso;
                mem.liberarProceso(nombre_proceso);
                break;
            case 3:
                mem.compactarMemoriaFisica();
                break;
            case 4:
                mem.mostrar();
                break;
            case 5:
                mem.calcularFragmentacionInterna();
                break;
            case 6:
                mem.calcularFragmentacionExterna();
                break;
            case 0:
                cout << "Saliendo del simulador...\n";
                break;
            default:
                cout << "Opción no válida. Intente de nuevo.\n";
        }
    } while (opcion != 0);

    return 0;
}
