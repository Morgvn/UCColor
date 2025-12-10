# UCColor - Aimbot basado en detección de color

## ⚠️ ADVERTENCIA IMPORTANTE

**Este software puede resultar en la suspensión permanente de tu cuenta. El uso es bajo tu propia responsabilidad. No me hago responsable de ningún ban o consecuencia derivada del uso de este software.**

## 📍 Ubicación del Programa

El ejecutable compilado se encuentra en:
- **`x64/Debug/UCAimColor.exe`** - Programa principal listo para usar

El código fuente completo está disponible en la carpeta **`UCAimColor/`** para que puedas modificarlo y compilarlo a tu gusto.

## 🚀 Uso Rápido

1. **Ejecutar como Administrador** (recomendado pero no obligatorio)
   - Haz clic derecho en `UCAimColor.exe` → "Ejecutar como administrador"

2. El programa funciona de dos formas:
   - **Con Driver** (opcional): Si tienes `Mapper.exe` y `Oykyo.sys` en la misma carpeta, el programa intentará cargarlos automáticamente para un funcionamiento más suave
   - **Sin Driver (Modo Blatant)**: Si no hay driver o falla la carga, el programa usa `SendInput` de Windows directamente. Este modo es más obvio pero funciona perfectamente sin necesidad de permisos de administrador ni drivers
   - ## 📝 Notas Técnicas

- El programa detecta automáticamente si el driver está disponible
- Si no hay driver, usa modo fallback con `SendInput` (más blatant pero funcional)
- Modo fallback: velocidad x6, smooth mínimo para máximo rendimiento
- El proyecto usa ImGui para la interfaz gráfica

## ⚠️ Riesgo de Ban

- **ALTO RIESGO**: El uso de este software puede resultar en ban permanente
- Los sistemas anti-cheat pueden detectar:
  - Movimientos anormales del mouse
  - Patrones de comportamiento sospechoso
  - El driver kernel (si se usa)
- **El ban es responsabilidad del usuario**

---

**El código fuente está disponible para que lo modifiques y adaptes a tus necesidades. Úsalo bajo tu propia responsabilidad.**
