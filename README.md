# ZMC - Z80 Managed Commander (Soft Edition)

**ZMC** es un gestor de archivos ligero y eficiente diseñado específicamente para sistemas operativos **CP/M** corriendo en procesadores **Z80**. Optimizado para terminales con emulación **VT100/VT102** de 32 líneas (como Minicom o hardware real).

---

## 🚀 Características (Features)
* **Dual Panel Interface:** Gestión clásica de archivos en dos paneles.
* **Multidrive Support:** Navegación instantánea entre unidades (A-Z).
* **High-Speed Refresh:** Refresco quirúrgico de pantalla para evitar parpadeos en conexiones seriales.
* **Batch Operations:** Selección múltiple de archivos (tecla Espacio) para copia en lote (F5).
* **Integrated Tools:**
    * **F3 (View):** Visor de texto con paginación inteligente. [cite: 2026-01-31]
    * **F4 (Dump):** Volcado hexadecimal y ASCII profesional. [cite: 2026-01-31]
    * **F8 (Delete):** Borrado con confirmación de seguridad. [cite: 2026-01-31]

## 🛠️ Requisitos de Compilación (Build Requirements)
Este proyecto utiliza el compilador **z88dk** en un entorno Linux.

```bash
# Para compilar, simplemente corre el script de taller:
./make.sh
