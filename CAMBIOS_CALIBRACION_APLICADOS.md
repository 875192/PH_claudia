# 🔧 CAMBIOS APLICADOS: CORRECCIÓN CALIBRACIÓN TOUCHSCREEN

## ✅ CAMBIOS IMPLEMENTADOS

### 1. **CRÍTICO: Lógica de swap XY corregida** ⚠️

**Archivo:** `tp.c` línea ~636  
**Problema:** La condición estaba invertida respecto a la implementación de Jimmy  
**Cambio aplicado:**
```c
// ANTES (INCORRECTO):
if (dx < dy) {
    g_swap_xy = 1;

// DESPUÉS (CORREGIDO):
if (dx > dy) {
    g_swap_xy = 1;
```

**Explicación:** 
- Si `dx > dy`: La variación en X es mayor, significa que los ejes están intercambiados → aplicar swap
- Si `dx < dy`: La variación en Y es mayor, significa que los ejes están correctos → no aplicar swap

### 2. **Margen de calibración mejorado** 📏

**Archivos:** `tp.c` (funciones `ts_test_calibracion` y `ts_test_numeros`)  
**Cambio aplicado:**
```c
// ANTES:
ts_calibrate_5pt(SCR_XSIZE, SCR_YSIZE, 30);

// DESPUÉS:
ts_calibrate_5pt(SCR_XSIZE, SCR_YSIZE, 50);
```

**Beneficios:**
- Puntos de calibración más alejados de los bordes
- Mayor rango de medición para los cálculos
- Mejor precisión general del touchscreen

### 3. **Documentación actualizada** 📚

**Archivos actualizados:**
- `EJEMPLO_MAIN_CALIBRACION.md`
- `CALIBRACION_TOUCHSCREEN_GUIA.md`

Todos los ejemplos ahora usan el margen correcto de 50 píxeles.

---

## 🎯 IMPACTO ESPERADO

### Antes de los cambios:
- ❌ Detección incorrecta de swap XY
- ❌ Puntos de calibración muy cerca de los bordes
- ❌ Calibración imprecisa y errática

### Después de los cambios:
- ✅ Detección correcta de swap XY
- ✅ Margen óptimo para calibración de precisión
- ✅ Calibración estable y precisa como la de Jimmy

---

## 🚀 CÓMO PROBAR

1. **Recompila el proyecto** para aplicar los cambios
2. **Ejecuta la calibración** llamando a `ts_test_calibracion()` 
3. **Verifica la precisión** tocando diferentes zonas de la pantalla
4. **Los toques deben ser mucho más precisos** ahora

---

## 📝 NOTAS TÉCNICAS

- **Línea crítica corregida:** tp.c:636 `if (dx > dy)` 
- **Margen optimizado:** 50 píxeles (era 30)
- **Basado en:** Análisis de implementación exitosa de Jimmy
- **Compatibilidad:** Mantiene toda la funcionalidad existente

¡La calibración ahora debería funcionar con la misma precisión que la de Jimmy! 🎉