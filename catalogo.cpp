/*
==============================================================================
  SISTEMA DE CATALOGO DE PRODUCTOS
==============================================================================
*/

#include <algorithm>
#include <climits>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

// ============================================================
//  ESTRUCTURA DE PRODUCTO
// ============================================================
struct Producto {
  int id;
  string nombre;
  long precio;
  int cantidad;
  string categoria;
  string tipo;
};

// ============================================================
//  NODO DE LISTA ENLAZADA DOBLE
// ============================================================
struct Nodo {
  Producto dato;
  Nodo *prev;
  Nodo *next;
  Nodo(const Producto &p) : dato(p), prev(nullptr), next(nullptr) {}
};

// ============================================================
//  LISTA ENLAZADA DOBLE
// ============================================================
class ListaDoble {
public:
  Nodo *head;
  Nodo *tail;
  int tam;

  ListaDoble() : head(nullptr), tail(nullptr), tam(0) {}

  ~ListaDoble() {
    Nodo *cur = head;
    while (cur) {
      Nodo *nx = cur->next;
      delete cur;
      cur = nx;
    }
  }

  void insertarAlFinal(const Producto &p) {
    Nodo *nuevo = new Nodo(p);
    if (!tail) {
      head = tail = nuevo;
    } else {
      nuevo->prev = tail;
      tail->next = nuevo;
      tail = nuevo;
    }
    tam++;
  }

  // Convierte la lista a vector (para ordenamiento)
  vector<Nodo *> aVector() const {
    vector<Nodo *> v;
    v.reserve(tam);
    for (Nodo *c = head; c; c = c->next)
      v.push_back(c);
    return v;
  }

  // Reconstruye la lista a partir de un vector ordenado
  void reconstruirDesdeVector(const vector<Nodo *> &v) {
    if (v.empty()) {
      head = tail = nullptr;
      return;
    }
    for (int i = 0; i < (int)v.size(); i++) {
      v[i]->prev = (i > 0) ? v[i - 1] : nullptr;
      v[i]->next = (i < (int)v.size() - 1) ? v[i + 1] : nullptr;
    }
    head = v.front();
    tail = v.back();
  }
};

// ============================================================
//  TABLA HASH: categoria -> vector de punteros a Nodo
// ============================================================
using TablaHash = unordered_map<string, vector<Nodo *>>;

// ============================================================
//  UTILIDADES DE CADENA
// ============================================================
string trim(const string &s) {
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == string::npos)
    return "";
  size_t b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b - a + 1);
}

string toLower(string s) {
  for (auto &c : s)
    c = tolower((unsigned char)c);
  return s;
}

// ============================================================
//  MERGE SORT SOBRE VECTOR DE NODOS*
// ============================================================
void merge(vector<Nodo *> &v, int l, int m, int r, bool porPrecio) {
  vector<Nodo *> tmp;
  int i = l, j = m + 1;
  while (i <= m && j <= r) {
    bool izqMenor =
        porPrecio ? v[i]->dato.precio <= v[j]->dato.precio
                  : toLower(v[i]->dato.nombre) <= toLower(v[j]->dato.nombre);
    if (izqMenor)
      tmp.push_back(v[i++]);
    else
      tmp.push_back(v[j++]);
  }
  while (i <= m)
    tmp.push_back(v[i++]);
  while (j <= r)
    tmp.push_back(v[j++]);
  for (int k = l; k <= r; k++)
    v[k] = tmp[k - l];
}

void mergeSort(vector<Nodo *> &v, int l, int r, bool porPrecio) {
  if (l >= r)
    return;
  int m = l + (r - l) / 2;
  mergeSort(v, l, m, porPrecio);
  mergeSort(v, m + 1, r, porPrecio);
  merge(v, l, m, r, porPrecio);
}

// ============================================================
//  FUNCIONES RECURSIVAS
// ============================================================

// Buscar por nombre (recursiva sobre lista)
void buscarNombreRec(Nodo *nodo, const string &clave,
                     vector<Nodo *> &resultados) {
  if (!nodo)
    return;
  if (toLower(nodo->dato.nombre).find(toLower(clave)) != string::npos)
    resultados.push_back(nodo);
  buscarNombreRec(nodo->next, clave, resultados);
}

// Imprimir Top-N (recursiva)
void imprimirTopRec(const vector<Nodo *> &v, int idx, int n) {
  if (idx >= n || idx >= (int)v.size())
    return;
  const Producto &p = v[idx]->dato;
  cout << "| " << setw(4) << left << p.id << " | " << setw(28) << left
       << p.nombre.substr(0, 28) << " | " << setw(12) << right << p.precio
       << " | " << setw(5) << right << p.cantidad << " | " << setw(12) << left
       << p.categoria << " | " << setw(14) << left << p.tipo << " |"
       << "\n";
  imprimirTopRec(v, idx + 1, n);
}

// ============================================================
//  INTERFAZ: DIBUJO DE TABLAS ASCII
// ============================================================
void cabeceraTablá() {
  cout << "+------+------------------------------+--------------+-------+------"
          "--------+----------------+\n";
  cout << "| ID   | Nombre                       | Precio (COP) | Cant. | "
          "Categoria    | Tipo           |\n";
  cout << "+------+------------------------------+--------------+-------+------"
          "--------+----------------+\n";
}

void pieTablá() {
  cout << "+------+------------------------------+--------------+-------+------"
          "--------+----------------+\n";
}

void imprimirNodo(const Nodo *n) {
  const Producto &p = n->dato;
  cout << "| " << setw(4) << left << p.id << " | " << setw(28) << left
       << p.nombre.substr(0, 28) << " | " << setw(12) << right << p.precio
       << " | " << setw(5) << right << p.cantidad << " | " << setw(12) << left
       << p.categoria << " | " << setw(14) << left << p.tipo << " |\n";
}

// ============================================================
//  CARGA DEL TXT
// ============================================================
bool cargarTXT(const string &archivo, ListaDoble &lista, TablaHash &tabla) {
  ifstream f(archivo);
  if (!f.is_open()) {
    cout << "[ERROR] No se pudo abrir: " << archivo << "\n";
    return false;
  }
  string linea;
  int cargados = 0;
  while (getline(f, linea)) {
    linea = trim(linea);
    if (linea.empty())
      continue;
    istringstream ss(linea);
    string tok;
    Producto p;
    try {
      getline(ss, tok, ',');
      p.id = stoi(trim(tok));
      getline(ss, tok, ',');
      p.nombre = trim(tok);
      getline(ss, tok, ',');
      p.precio = stol(trim(tok));
      getline(ss, tok, ',');
      p.cantidad = stoi(trim(tok));
      getline(ss, tok, ',');
      p.categoria = trim(tok);
      getline(ss, tok, ',');
      p.tipo = trim(tok);
    } catch (...) {
      continue;
    }
    lista.insertarAlFinal(p);
    tabla[p.categoria].push_back(lista.tail);
    cargados++;
  }
  cout << "[OK] " << cargados << " productos cargados desde '" << archivo
       << "'.\n";
  return true;
}

// ============================================================
//  MENU PRINCIPAL
// ============================================================
void mostrarMenu() {
  cout << "\n";
  cout << "=================================================================\n";
  cout << "           SISTEMA DE CATALOGO DE PRODUCTOS v1.0                \n";
  cout << "=================================================================\n";
  cout << "  [1] Mostrar catalogo completo                                 \n";
  cout << "  [2] Agregar producto manualmente                              \n";
  cout << "  [3] Buscar por nombre (recursivo)                             \n";
  cout << "  [4] Filtrar por categoria (Hash O(1))                         \n";
  cout << "  [5] Ordenar catalogo por precio (Merge Sort)                  \n";
  cout << "  [6] Ordenar catalogo por nombre (Merge Sort)                  \n";
  cout << "  [7] Top-N productos por precio (recursivo)                    \n";
  cout << "  [8] Estadisticas del catalogo                                 \n";
  cout << "  [0] Salir                                                     \n";
  cout << "-----------------------------------------------------------------\n";
  cout << "  Seleccione una opcion: ";
}

// ============================================================
//  OPCION 1: Mostrar catalogo completo
// ============================================================
void mostrarCatalogo(const ListaDoble &lista) {
  if (!lista.head) {
    cout << "[INFO] El catalogo esta vacio.\n";
    return;
  }
  cout << "\n--- CATALOGO COMPLETO (" << lista.tam << " productos) ---\n";
  cabeceraTablá();
  for (Nodo *c = lista.head; c; c = c->next)
    imprimirNodo(c);
  pieTablá();
}

// ============================================================
//  OPCION 2: Agregar producto (BLINDADA)
// ============================================================
void agregarProducto(ListaDoble &lista, TablaHash &tabla) {
  Producto p;
  p.id = lista.tam + 1;

  cout << "\n--- AGREGAR NUEVO PRODUCTO (ID asignado: " << p.id << ") ---\n";
  cout << "Nombre: ";

  // Limpiamos el buffer antes de leer el string para evitar saltos de linea
  // fantasma
  cin.ignore(numeric_limits<streamsize>::max(), '\n');
  getline(cin, p.nombre);
  p.nombre = trim(p.nombre);

  cout << "Precio (COP, entero): ";
  // Ciclo que atrapa letras o numeros negativos
  while (!(cin >> p.precio) || p.precio < 0) {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "[ERROR] Entrada invalida. Ingrese un precio numerico positivo: ";
  }

  cout << "Cantidad: ";
  while (!(cin >> p.cantidad) || p.cantidad < 0) {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout
        << "[ERROR] Entrada invalida. Ingrese una cantidad numerica positiva: ";
  }

  cout << "\nCategorias disponibles:\n";
  cout << "  [1] Electronica   [2] Ropa   [3] Hogar   [4] Deportes   [5] "
          "Alimentos\n";

  int opCat;
  cout << "Categoria: ";
  // Validamos que sea un numero y que este entre 1 y 5
  while (!(cin >> opCat) || opCat < 1 || opCat > 5) {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "[ERROR] Invalido. Seleccione una categoria entre 1 y 5: ";
  }

  vector<string> tipos;
  switch (opCat) {
  case 1:
    p.categoria = "Electronica";
    tipos = {"Computador", "Celular", "Accesorio"};
    break;
  case 2:
    p.categoria = "Ropa";
    tipos = {"Camisa", "Pantalon", "Calzado"};
    break;
  case 3:
    p.categoria = "Hogar";
    tipos = {"Mueble", "Electrodomestico", "Decoracion"};
    break;
  case 4:
    p.categoria = "Deportes";
    tipos = {"Equipamiento", "Ropa Deportiva", "Accesorio"};
    break;
  case 5:
    p.categoria = "Alimentos";
    tipos = {"Perecedero", "No Perecedero", "Bebida"};
    break;
  }

  cout << "Tipos para " << p.categoria << ":\n";
  for (int i = 0; i < (int)tipos.size(); i++)
    cout << "  [" << i + 1 << "] " << tipos[i] << "\n";

  int opTipo;
  cout << "Tipo: ";
  // Validamos dinamicamente segun la cantidad de tipos disponibles
  while (!(cin >> opTipo) || opTipo < 1 || opTipo > (int)tipos.size()) {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "[ERROR] Invalido. Seleccione un tipo entre 1 y " << tipos.size()
         << ": ";
  }
  p.tipo = tipos[opTipo - 1];

  lista.insertarAlFinal(p);
  tabla[p.categoria].push_back(lista.tail);
  cout << "[OK] Producto '" << p.nombre << "' agregado con ID " << p.id
       << ".\n";
}

// ============================================================
//  OPCION 3: Buscar por nombre (recursivo)
// ============================================================
void buscarPorNombre(const ListaDoble &lista) {
  cout << "Texto a buscar: ";
  cin.ignore(numeric_limits<streamsize>::max(), '\n');
  string clave;
  getline(cin, clave);
  clave = trim(clave);

  vector<Nodo *> res;
  buscarNombreRec(lista.head, clave, res);

  if (res.empty()) {
    cout << "[INFO] Sin resultados para '" << clave << "'.\n";
    return;
  }
  cout << "\n--- RESULTADOS para '" << clave << "' (" << res.size()
       << " encontrados) ---\n";
  cabeceraTablá();
  for (auto n : res)
    imprimirNodo(n);
  pieTablá();
}

// ============================================================
//  OPCION 4: Filtrar por categoria (Hash O(1))
// ============================================================
void filtrarPorCategoria(const TablaHash &tabla) {
  cout << "\nCategorias disponibles:\n";
  cout << "  [1] Electronica   [2] Ropa   [3] Hogar   [4] Deportes   [5] "
          "Alimentos\n";
  int op;
  cout << "Categoria: ";
  // Proteccion basica
  while (!(cin >> op) || op < 1 || op > 5) {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "[ERROR] Opcion invalida. Intente de nuevo: ";
  }

  string cat;
  switch (op) {
  case 1:
    cat = "Electronica";
    break;
  case 2:
    cat = "Ropa";
    break;
  case 3:
    cat = "Hogar";
    break;
  case 4:
    cat = "Deportes";
    break;
  case 5:
    cat = "Alimentos";
    break;
  }

  auto it = tabla.find(cat);
  if (it == tabla.end() || it->second.empty()) {
    cout << "[INFO] No hay productos en '" << cat << "'.\n";
    return;
  }
  cout << "\n--- CATEGORIA: " << cat << " (" << it->second.size()
       << " productos) ---\n";
  cabeceraTablá();
  for (auto n : it->second)
    imprimirNodo(n);
  pieTablá();
}

// ============================================================
//  OPCION 5/6: Ordenar catalogo
// ============================================================
void ordenarCatalogo(ListaDoble &lista, bool porPrecio) {
  if (lista.tam < 2) {
    cout << "[INFO] No hay suficientes productos para ordenar.\n";
    return;
  }
  vector<Nodo *> v = lista.aVector();
  mergeSort(v, 0, v.size() - 1, porPrecio);
  lista.reconstruirDesdeVector(v);
  string criterio = porPrecio ? "precio" : "nombre";
  cout << "[OK] Catalogo ordenado por " << criterio
       << " (Merge Sort O(n log n)).\n";
  mostrarCatalogo(lista);
}

// ============================================================
//  OPCION 7: Top-N por precio (recursivo - BLINDADA)
// ============================================================
void topNPorPrecio(ListaDoble &lista) {
  int n;
  cout << "Cuantos productos mostrar (N): ";

  // Protegemos contra letras o numeros menores o iguales a 0
  while (!(cin >> n) || n <= 0) {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "[ERROR] Entrada invalida. Por favor ingrese un numero entero "
            "mayor a 0: ";
  }

  if (lista.tam == 0) {
    cout << "[INFO] El catalogo esta vacio.\n";
    return;
  }

  // Ordenar descendente por precio para el top
  vector<Nodo *> v = lista.aVector();
  mergeSort(v, 0, v.size() - 1, true);
  reverse(v.begin(), v.end());

  int mostrar = min(n, (int)v.size());
  cout << "\n--- TOP-" << mostrar << " PRODUCTOS MAS CAROS ---\n";
  cabeceraTablá();
  imprimirTopRec(v, 0, mostrar);
  pieTablá();
}

// ============================================================
//  OPCION 8: Estadisticas
// ============================================================
void mostrarEstadisticas(const ListaDoble &lista, const TablaHash &tabla) {
  if (!lista.head) {
    cout << "[INFO] Catalogo vacio.\n";
    return;
  }
  long totalInv = 0, maxP = 0, minP = LONG_MAX;
  int sinStock = 0;
  for (Nodo *c = lista.head; c; c = c->next) {
    totalInv += (long)c->dato.precio * c->dato.cantidad;
    if (c->dato.precio > maxP)
      maxP = c->dato.precio;
    if (c->dato.precio < minP)
      minP = c->dato.precio;
    if (c->dato.cantidad == 0)
      sinStock++;
  }
  cout << "\n=== ESTADISTICAS ===\n";
  cout << "  Total productos    : " << lista.tam << "\n";
  cout << "  Precio maximo      : " << maxP << " COP\n";
  cout << "  Precio minimo      : " << minP << " COP\n";
  cout << "  Sin stock (cant=0) : " << sinStock << "\n";
  cout << "  Valor total inv.   : " << totalInv << " COP\n\n";
  cout << "  Distribucion por categoria:\n";
  for (auto &kv : tabla)
    cout << "    " << setw(14) << left << kv.first << ": " << kv.second.size()
         << " productos\n";
}

// ============================================================
//  PROGRAMA PRINCIPAL
// ============================================================
int main() {
  ListaDoble lista;
  TablaHash tabla;

  cout << "=================================================================\n";
  cout << "  SISTEMA DE CATALOGO DE PRODUCTOS - Ingreso de datos           \n";
  cout << "=================================================================\n";
  cout << "  Nombre del archivo TXT (ej. BaseDeDatos.txt): ";
  string archivo;
  cin >> archivo;

  if (!cargarTXT(archivo, lista, tabla)) {
    cout << "[INFO] Iniciando con catalogo vacio.\n";
  }

  int opcion = -1;
  while (opcion != 0) {
    mostrarMenu();

    // --- BLOQUE CORREGIDO: Proteccion del menu principal ---
    if (!(cin >> opcion)) {
      cin.clear(); // Limpia el estado de error de cin
      cin.ignore(numeric_limits<streamsize>::max(),
                 '\n'); // Descarta la entrada mala
      opcion = -1; // Reiniciamos la variable para evitar que sea 0 por el error
      cout << "\n[ERROR] Entrada invalida. Por favor ingrese un numero.\n";
      continue;
    }
    // -------------------------------------------------------

    switch (opcion) {
    case 1:
      mostrarCatalogo(lista);
      break;
    case 2:
      agregarProducto(lista, tabla);
      break;
    case 3:
      buscarPorNombre(lista);
      break;
    case 4:
      filtrarPorCategoria(tabla);
      break;
    case 5:
      ordenarCatalogo(lista, true);
      break;
    case 6:
      ordenarCatalogo(lista, false);
      break;
    case 7:
      topNPorPrecio(lista);
      break;
    case 8:
      mostrarEstadisticas(lista, tabla);
      break;
    case 0:
      cout << "\n[BYE] Hasta luego.\n";
      break;
    default:
      cout << "[ERROR] Opcion invalida.\n";
    }
  }
  return 0;
}