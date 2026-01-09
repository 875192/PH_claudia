################################################################################
# IMPLEMENTACIONES ARM PARA SUDOKU
# Archivo: init_b.asm
# 
# Este archivo contiene las implementaciones en ensamblador ARM de las
# funciones del sudoku optimizadas a nivel de instrucciones.
# El código de arranque se gestiona desde main.c
################################################################################

.text
.arm    /* indicates that we are using the ARM instruction set */

################################################################################
# IMPLEMENTACIÓN ARM DE candidatos_propagar_arm
#
# Función: candidatos_propagar_arm
# Parámetros:
#   r0: CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS] - puntero base cuadrícula
#   r1: uint8_t fila - fila de la celda a propagar (0-8)
#   r2: uint8_t columna - columna de la celda a propagar (0-8)
#
# La función lee el valor de la celda y propaga eliminando ese candidato
# de todas las celdas en:
# - La misma fila
# - La misma columna
# - La misma región 3x3
################################################################################
.global candidatos_propagar_arm

candidatos_propagar_arm:
    # Preservar registros
    STMDB   SP!, {r4-r10, fp, LR}
    ADD     fp, SP, #(4*7-4)    @ FP ← dirección base del marco (7 registros: r4-r10)

    # ========== ASIGNACIÓN DE REGISTROS ==========
    # REGISTROS CRÍTICOS (no reutilizables):
    # r0 = puntero base cuadrícula (INMUTABLE)
    # r1 = fila (parámetro)
    # r2 = columna (parámetro)
    # r3 = valor a propagar (calculado)
    # r4 = NUM_FILAS = 9 (constante)
    # r5 = NUM_COLUMNAS = 16 (constante)
    # r10 = máscara para eliminar candidato (calculada)
    #
    # REGISTROS REUTILIZABLES:
    # r6 = contador j (bucle fila / región)
    # r7 = contador i (bucle columna / región)
    # r8 = dirección de celda temporal
    # r9 = valor de celda temporal

    # Inicializar constantes
    MOV     r4, #9          // NUM_FILAS
    MOV     r5, #16         // NUM_COLUMNAS (9 + 7 padding)

    # Leer valor de la celda actual: cuadricula[fila][columna]
    MLA     r8, r1, r5, r2  // r8 = fila * NUM_COLUMNAS + columna
    MOV     r8, r8, LSL #1  // r8 *= 2 (sizeof CELDA)
    ADD     r8, r0, r8      // r8 = dirección de cuadricula[fila][columna]
    LDRH    r3, [r8]        // r3 = cuadricula[fila][columna]
    AND     r3, r3, #0x0F   // r3 = valor (bits 3-0)

    # Si valor es 0, no hay nada que propagar
    CMP     r3, #0
    BEQ     propagar_fin

    # Calcular máscara para eliminar candidato: (1 << (7 + valor - 1))
    ADD     r9, r3, #7      // r9 = 7 + valor (BIT_CANDIDATOS = 7)
    SUB     r9, r9, #1      // r9 = 7 + valor - 1
    MOV     r10, #1         // r10 = 1
    MOV     r10, r10, LSL r9 // r10 = máscara para eliminar candidato

    # ===== RECORRER FILA =====
    MOV     r6, #0          // j = 0

bucle_fila:
    CMP     r6, r4          // comparar j con NUM_FILAS
    BGE     fin_fila        // si j >= NUM_FILAS, terminar

    # Calcular dirección: cuadricula[fila][j]
    MLA     r8, r1, r5, r6  // r8 = fila * NUM_COLUMNAS + j
    MOV     r8, r8, LSL #1  // r8 *= 2 (sizeof CELDA)
    ADD     r8, r0, r8      // r8 = dirección de cuadricula[fila][j]

    # Leer valor de la celda
    LDRH    r9, [r8]        // r9 = cuadricula[fila][j]

    # Eliminar candidato: celda |= máscara
    ORR     r9, r9, r10     // r9 |= máscara
    STRH    r9, [r8]        // guardar celda modificada

siguiente_fila:
    ADD     r6, r6, #1      // j++
    B       bucle_fila

fin_fila:
    # ===== RECORRER COLUMNA =====
    MOV     r7, #0          // i = 0

bucle_columna:
    CMP     r7, r4          // comparar i con NUM_FILAS, evitamos padding
    BGE     fin_columna     // si i >= NUM_FILAS, terminar

    # Calcular dirección: cuadricula[i][columna]
    MLA     r8, r7, r5, r2  // r8 = i * NUM_COLUMNAS + columna
    MOV     r8, r8, LSL #1  // r8 *= 2 (sizeof CELDA)
    ADD     r8, r0, r8      // r8 = dirección de cuadricula[i][columna]

    # Leer valor de la celda
    LDRH    r9, [r8]        // r9 = cuadricula[i][columna]

    # Eliminar candidato: celda |= máscara
    ORR     r9, r9, r10     // r9 |= máscara
    STRH    r9, [r8]        // guardar celda modificada

siguiente_columna:
    ADD     r7, r7, #1      // i++
    B       bucle_columna

fin_columna:
    # ===== RECORRER REGIÓN 3x3 =====
    # Implementar lookup table: init_region[9] = {0, 0, 0, 3, 3, 3, 6, 6, 6}
    # Calcular init_i usando lookup table
    CMP     r1, #3
    MOVLT   r7, #0          // si fila < 3, init_i = 0
    MOVGE   r7, #3          // si fila >= 3, init_i = 3
    CMP     r1, #6
    MOVGE   r7, #6          // si fila >= 6, init_i = 6

    # Calcular init_j usando lookup table
    CMP     r2, #3
    MOVLT   r6, #0          // si columna < 3, init_j = 0
    MOVGE   r6, #3          // si columna >= 3, init_j = 3
    CMP     r2, #6
    MOVGE   r6, #6          // si columna >= 6, init_j = 6

    # r7 = init_i, r6 = init_j
    # Bucle región: for (i = init_i; i < init_i + 3; i++)
    ADD     r3, r7, #3      // r3 = init_i + 3 (reutilizar r3)

bucle_region_i:
    CMP     r7, r3          // comparar i con init_i + 3
    BGE     fin_region      // si i >= init_i + 3, terminar

    # Bucle región: for (j = init_j; j < init_j + 3; j++)
    # Recalcular init_j para bucle interno
    CMP     r2, #3
    MOVLT   r6, #0
    MOVGE   r6, #3
    CMP     r2, #6
    MOVGE   r6, #6
    
    ADD     r9, r6, #3      // r9 = init_j + 3

bucle_region_j:
    CMP     r6, r9          // comparar j con init_j + 3
    BGE     fin_region_j    // si j >= init_j + 3, terminar bucle j

    # Calcular dirección: cuadricula[i][j]
    MLA     r8, r7, r5, r6  // r8 = i * NUM_COLUMNAS + j
    MOV     r8, r8, LSL #1  // r8 *= 2 (sizeof CELDA)
    ADD     r8, r0, r8      // r8 = dirección de cuadricula[i][j]

    # Leer valor de la celda
    LDRH    r4, [r8]        // r4 = cuadricula[i][j] (reutilizar r4)

    # Eliminar candidato: celda |= máscara
    ORR     r4, r4, r10     // r4 |= máscara
    STRH    r4, [r8]        // guardar celda modificada

siguiente_region_j:
    ADD     r6, r6, #1      // j++
    B       bucle_region_j

fin_region_j:
    ADD     r7, r7, #1      // i++
    B       bucle_region_i

fin_region:
propagar_fin:
    # Restaurar registros y retornar
    LDMIA   SP!, {r4-r10, fp, LR}
    BX      LR


################################################################################
# IMPLEMENTACIÓN ARM DE candidatos_actualizar_arm_arm
#
# Función: candidatos_actualizar_arm_arm
# Parámetros:
#   r0: CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS] - puntero base cuadrícula
#
# Comportamiento:
# FASE 1: Recorre toda la cuadrícula y para cada celda:
#         - Limpia bits candidatos [15,7] poniendo todos a 0
#         - Si valor=0, incrementa contador de celdas vacías
# FASE 2: Recorre toda la cuadrícula y para cada celda con valor (valor≠0):
#         - Llama a candidatos_propagar_arm para eliminar candidatos
#
# Retorna: número de celdas vacías (en r0)
################################################################################
.global candidatos_actualizar_arm_arm

candidatos_actualizar_arm_arm:
    # Preservar registros y configurar frame pointer
    STMDB   SP!, {r4-r10, fp, LR}
    ADD     fp, SP, #(4*7-4)    @ FP ← dirección base del marco (7 registros: r4-r10)
    SUB     SP, SP, #8      // Reservar espacio para 2 variables locales (i, j)

    # ========== ASIGNACIÓN DE REGISTROS ==========
    # REGISTROS CRÍTICOS (no reutilizables):
    # r4 = puntero base cuadrícula (INMUTABLE)
    # r5 = NUM_FILAS = 9 (constante)
    # r6 = NUM_COLUMNAS = 16 (constante)
    # r7 = máscara para limpiar candidatos = 0x007F (constante)
    # r8 = contador de celdas vacías
    # 
    # REGISTROS REUTILIZABLES:
    # r9 = contador i (filas)
    # r10 = contador j (columnas)
    # r0,r1,r2,r3 = direcciones, valores temporales

    # Inicializar constantes
    MOV     r4, r0          // r4 = puntero base cuadrícula
    MOV     r5, #9          // r5 = NUM_FILAS
    MOV     r6, #16         // r6 = NUM_COLUMNAS (9 + 7 padding)
    MOV     r7, #0x007F     // r7 = máscara para limpiar candidatos (preservar bits 6-0)
    MOV     r8, #0          // r8 = contador celdas vacías

    # ===== FASE 1: LIMPIAR CANDIDATOS DE TODAS LAS CELDAS Y CONTAR CELDAS VACÍAS =====
    MOV     r9, #0          // r9 = i (contador filas)

fase1_arm_arm_bucle_i:
    CMP     r9, r5          // comparar i con NUM_FILAS
    BGE     fase1_arm_arm_fin // si i >= NUM_FILAS, terminar fase 1

    MOV     r10, #0         // r10 = j (contador columnas)

fase1_arm_arm_bucle_j:
    CMP     r10, r5         // comparar j con NUM_FILAS (solo primeras 9 columnas)
    BGE     fase1_arm_arm_fin_j // si j >= NUM_FILAS, terminar bucle j

    # Calcular dirección: cuadricula[i][j]
    MLA     r0, r9, r6, r10 // r0 = i * NUM_COLUMNAS + j
    MOV     r0, r0, LSL #1  // r0 *= 2 (sizeof CELDA)
    ADD     r0, r4, r0      // r0 = dirección de cuadricula[i][j]

    # Leer celda y limpiar candidatos de TODAS las celdas
    LDRH    r1, [r0]        // r1 = cuadricula[i][j]
    AND     r1, r1, r7      // r1 &= 0x007F (limpiar bits candidatos 15-7)
    STRH    r1, [r0]        // guardar celda modificada
    
    # Si valor es 0, incrementar contador de celdas vacías
    AND     r2, r1, #0x0F   // r2 = valor de la celda (bits 3-0)
    CMP     r2, #0          // ¿valor == 0?
    ADDEQ   r8, r8, #1      // si sí, celdas_vacias++

fase1_arm_arm_siguiente_j:
    ADD     r10, r10, #1    // j++
    B       fase1_arm_arm_bucle_j

fase1_arm_arm_fin_j:
    ADD     r9, r9, #1      // i++
    B       fase1_arm_arm_bucle_i

fase1_arm_arm_fin:
    # ===== FASE 2: PROPAGAR VALORES DE CELDAS NO VACÍAS =====
    MOV     r9, #0          // r9 = i (contador filas)

fase2_arm_arm_bucle_i:
    CMP     r9, r5          // comparar i con NUM_FILAS
    BGE     fase2_arm_arm_fin // si i >= NUM_FILAS, terminar fase 2

    MOV     r10, #0         // r10 = j (contador columnas)

fase2_arm_arm_bucle_j:
    CMP     r10, r5         // comparar j con NUM_FILAS (solo primeras 9 columnas)
    BGE     fase2_arm_arm_fin_j // si j >= NUM_FILAS, terminar bucle j

    # Calcular dirección: cuadricula[i][j]
    MLA     r0, r9, r6, r10 // r0 = i * NUM_COLUMNAS + j
    MOV     r0, r0, LSL #1  // r0 *= 2 (sizeof CELDA)
    ADD     r0, r4, r0      // r0 = dirección de cuadricula[i][j]

    # Procesar celda
    LDRH    r1, [r0]        // r1 = cuadricula[i][j]
    AND     r2, r1, #0x0F   // r2 = valor de la celda (bits 3-0)

    # Si celda está vacía (valor == 0), saltar
    CMP     r2, #0
    BEQ     fase2_arm_arm_siguiente_j

    # Celda con valor: llamar a candidatos_propagar_arm
    # Guardar estado actual usando frame pointer
    STR     r9, [fp, #-4]   // guardar i (fila)
    STR     r10, [fp, #-8]  // guardar j (columna)
    
    # Preparar parámetros para la llamada
    MOV     r0, r4          // r0 = puntero base cuadrícula
    MOV     r1, r9          // r1 = fila (i)
    MOV     r2, r10         // r2 = columna (j)

    # Llamar a candidatos_propagar_arm
    BL      candidatos_propagar_arm

    # Restaurar estado desde frame pointer
    LDR     r9, [fp, #-4]   // restaurar i (fila)
    LDR     r10, [fp, #-8]  // restaurar j (columna)

fase2_arm_arm_siguiente_j:
    ADD     r10, r10, #1    // j++
    B       fase2_arm_arm_bucle_j

fase2_arm_arm_fin_j:
    ADD     r9, r9, #1      // i++
    B       fase2_arm_arm_bucle_i

fase2_arm_arm_fin:
    # Preparar valor de retorno
    MOV     r0, r8          // r0 = celdas_vacias (valor de retorno)

    # Restaurar pila y registros, luego retornar
    ADD     SP, SP, #8      // Liberar espacio de variables locales
    LDMIA   SP!, {r4-r10, fp, LR}
    BX      LR

################################################################################
# IMPLEMENTACIÓN ARM DE candidatos_actualizar_arm_c
#
# Función: candidatos_actualizar_arm_c
# Parámetros:
#   r0: CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS] - puntero base cuadrícula
#
# Comportamiento:
# FASE 1: Recorre toda la cuadrícula y para cada celda:
#         - Limpia bits candidatos [15,7] poniendo todos a 0
#         - Si valor=0, incrementa contador de celdas vacías
# FASE 2: Recorre toda la cuadrícula y para cada celda con valor (valor≠0):
#         - Llama a candidatos_propagar_c (función C) para eliminar candidatos
#
# Retorna: número de celdas vacías (en r0)
################################################################################
.global candidatos_actualizar_arm_c

candidatos_actualizar_arm_c:
    # Preservar registros y configurar frame pointer
    STMDB   SP!, {r4-r10, fp, LR}
    ADD     fp, SP, #(4*7-4)    @ FP ← dirección base del marco (7 registros: r4-r10)
    SUB     SP, SP, #8      // Reservar espacio para 2 variables locales (i, j)

    # ========== ASIGNACIÓN DE REGISTROS ==========
    # REGISTROS CRÍTICOS (no reutilizables):
    # r4 = puntero base cuadrícula (INMUTABLE)
    # r5 = NUM_FILAS = 9 (constante)
    # r6 = NUM_COLUMNAS = 16 (constante)
    # r7 = máscara para limpiar candidatos = 0x007F (constante)
    # r8 = contador de celdas vacías
    # 
    # REGISTROS REUTILIZABLES:
    # r9 = contador i (filas)
    # r10 = contador j (columnas)
    # r0,r1,r2,r3 = direcciones, valores temporales

    # Inicializar constantes
    MOV     r4, r0          // r4 = puntero base cuadrícula
    MOV     r5, #9          // r5 = NUM_FILAS
    MOV     r6, #16         // r6 = NUM_COLUMNAS (9 + 7 padding)
    MOV     r7, #0x007F     // r7 = máscara para limpiar candidatos (preservar bits 6-0)
    MOV     r8, #0          // r8 = contador celdas vacías

    # ===== FASE 1: LIMPIAR CANDIDATOS DE TODAS LAS CELDAS Y CONTAR CELDAS VACÍAS =====
    MOV     r9, #0          // r9 = i (contador filas)

fase1_arm_c_bucle_i:
    CMP     r9, r5          // comparar i con NUM_FILAS
    BGE     fase1_arm_c_fin // si i >= NUM_FILAS, terminar fase 1

    MOV     r10, #0         // r10 = j (contador columnas)

fase1_arm_c_bucle_j:
    CMP     r10, r5         // comparar j con NUM_FILAS (solo primeras 9 columnas)
    BGE     fase1_arm_c_fin_j // si j >= NUM_FILAS, terminar bucle j

    # Calcular dirección: cuadricula[i][j]
    MLA     r0, r9, r6, r10 // r0 = i * NUM_COLUMNAS + j
    MOV     r0, r0, LSL #1  // r0 *= 2 (sizeof CELDA)
    ADD     r0, r4, r0      // r0 = dirección de cuadricula[i][j]

    # Leer celda y limpiar candidatos de TODAS las celdas
    LDRH    r1, [r0]        // r1 = cuadricula[i][j]
    AND     r1, r1, r7      // r1 &= 0x007F (limpiar bits candidatos 15-7)
    STRH    r1, [r0]        // guardar celda modificada
    
    # Si valor es 0, incrementar contador de celdas vacías
    AND     r2, r1, #0x0F   // r2 = valor de la celda (bits 3-0)
    CMP     r2, #0          // ¿valor == 0?
    ADDEQ   r8, r8, #1      // si sí, celdas_vacias++

fase1_arm_c_siguiente_j:
    ADD     r10, r10, #1    // j++
    B       fase1_arm_c_bucle_j

fase1_arm_c_fin_j:
    ADD     r9, r9, #1      // i++
    B       fase1_arm_c_bucle_i

fase1_arm_c_fin:
    # ===== FASE 2: PROPAGAR VALORES DE CELDAS NO VACÍAS =====
    MOV     r9, #0          // r9 = i (contador filas)

fase2_arm_c_bucle_i:
    CMP     r9, r5          // comparar i con NUM_FILAS
    BGE     fase2_arm_c_fin // si i >= NUM_FILAS, terminar fase 2

    MOV     r10, #0         // r10 = j (contador columnas)

fase2_arm_c_bucle_j:
    CMP     r10, r5         // comparar j con NUM_FILAS (solo primeras 9 columnas)
    BGE     fase2_arm_c_fin_j // si j >= NUM_FILAS, terminar bucle j

    # Calcular dirección: cuadricula[i][j]
    MLA     r0, r9, r6, r10 // r0 = i * NUM_COLUMNAS + j
    MOV     r0, r0, LSL #1  // r0 *= 2 (sizeof CELDA)
    ADD     r0, r4, r0      // r0 = dirección de cuadricula[i][j]

    # Procesar celda
    LDRH    r1, [r0]        // r1 = cuadricula[i][j]
    AND     r2, r1, #0x0F   // r2 = valor de la celda (bits 3-0)

    # Si celda está vacía (valor == 0), saltar
    CMP     r2, #0
    BEQ     fase2_arm_c_siguiente_j

    # Celda con valor: llamar a candidatos_propagar_c
    # Guardar estado actual usando frame pointer
    STR     r9, [fp, #-4]   // guardar i (fila)
    STR     r10, [fp, #-8]  // guardar j (columna)
    
    # Preparar parámetros para la llamada
    MOV     r0, r4          // r0 = puntero base cuadrícula
    MOV     r1, r9          // r1 = fila (i)
    MOV     r2, r10         // r2 = columna (j)

    # Llamar a candidatos_propagar_c (función C)
    BL      candidatos_propagar_c

    # Restaurar estado desde frame pointer
    LDR     r9, [fp, #-4]   // restaurar i (fila)
    LDR     r10, [fp, #-8]  // restaurar j (columna)

fase2_arm_c_siguiente_j:
    ADD     r10, r10, #1    // j++
    B       fase2_arm_c_bucle_j

fase2_arm_c_fin_j:
    ADD     r9, r9, #1      // i++
    B       fase2_arm_c_bucle_i

fase2_arm_c_fin:
    # Preparar valor de retorno
    MOV     r0, r8          // r0 = celdas_vacias (valor de retorno)

    # Restaurar pila y registros, luego retornar
    ADD     SP, SP, #8      // Liberar espacio de variables locales
    LDMIA   SP!, {r4-r10, fp, LR}
    BX      LR

################################################################################
# IMPLEMENTACIÓN ARM DE candidatos_actualizar_all
#
# Función: candidatos_actualizar_all
# Parámetros:
#   r0: CELDA cuadricula[NUM_FILAS][NUM_COLUMNAS] - puntero base cuadrícula
#
# Comportamiento:
# FASE 1: Recorre toda la cuadrícula y para cada celda:
#         - Limpia bits candidatos [15,7] poniendo todos a 0
#         - Si valor=0, incrementa contador de celdas vacías
# FASE 2: Recorre toda la cuadrícula y para cada celda con valor (valor≠0):
#         - Ejecuta código inlineado de candidatos_propagar_arm para eliminar candidatos
#           (evitando la sobrecarga de llamadas a subrutina)
#
# Retorna: número de celdas vacías (en r0)
################################################################################
.global candidatos_actualizar_all

candidatos_actualizar_all:
    # Preservar registros y configurar frame pointer
    STMDB   SP!, {r4-r10, fp, LR}
    ADD     fp, SP, #(4*7-4)    @ FP ← dirección base del marco (7 registros: r4-r10)
    SUB     SP, SP, #8      // Reservar espacio para 2 variables locales (i, j)

    # ========== ASIGNACIÓN DE REGISTROS ==========
    # REGISTROS CRÍTICOS (no reutilizables):
    # r4 = puntero base cuadrícula (INMUTABLE)
    # r5 = NUM_FILAS = 9 (constante)
    # r6 = NUM_COLUMNAS = 16 (constante)  
    # r7 = contador de celdas vacías (CRÍTICO para retorno)
    # 
    # REGISTROS REUTILIZABLES:
    # r8 = contador i principal / contador bucles internos
    # r9 = contador j principal / direcciones temporales
    # r10 = dirección celda actual / máscaras temporales / valores temporales

    # Inicializar registros críticos
    MOV     r4, r0          // r4 = puntero base cuadrícula (inmutable)
    MOV     r5, #9          // r5 = NUM_FILAS
    MOV     r6, #16         // r6 = NUM_COLUMNAS (9 + 7 padding)
    MOV     r7, #0          // r7 = contador celdas vacías

    # ===== FASE 1: LIMPIAR CANDIDATOS Y CONTAR CELDAS VACÍAS =====
    MOV     r8, #0          // r8 = i (contador filas)

fase1_all_bucle_i:
    CMP     r8, r5          // comparar i con NUM_FILAS
    BGE     fase1_all_fin   // si i >= NUM_FILAS, terminar fase 1

    MOV     r9, #0          // r9 = j (contador columnas)

fase1_all_bucle_j:
    CMP     r9, r5          // comparar j con NUM_FILAS (solo primeras 9 columnas)
    BGE     fase1_all_fin_j // si j >= NUM_FILAS, terminar bucle j

    # Calcular dirección: cuadricula[i][j]
    MLA     r10, r8, r6, r9 // r10 = i * NUM_COLUMNAS + j
    MOV     r10, r10, LSL #1 // r10 *= 2 (sizeof CELDA)
    ADD     r10, r4, r10    // r10 = dirección de cuadricula[i][j]

    # Leer celda y procesar
    LDRH    r0, [r10]       // r0 = cuadricula[i][j]
    
    # Limpiar bits candidatos [15,7] manteniendo [6,0]
    AND     r0, r0, #0x007F // r0 &= 0x007F (limpiar candidatos)
    STRH    r0, [r10]       // guardar celda modificada
    
    # Si valor es 0, incrementar contador de celdas vacías
    AND     r0, r0, #0x0F   // r0 = valor de la celda (bits 3-0)
    CMP     r0, #0          // ¿valor == 0?
    ADDEQ   r7, r7, #1      // si sí, celdas_vacias++

fase1_all_siguiente_j:
    ADD     r9, r9, #1      // j++
    B       fase1_all_bucle_j

fase1_all_fin_j:
    ADD     r8, r8, #1      // i++
    B       fase1_all_bucle_i

fase1_all_fin:
    # ===== FASE 2: PROPAGAR VALORES DE CELDAS NO VACÍAS =====
    MOV     r8, #0          // r8 = i (contador filas principales)

fase2_all_bucle_i:
    CMP     r8, r5          // comparar i con NUM_FILAS
    BGE     fase2_all_fin   // si i >= NUM_FILAS, terminar fase 2

    MOV     r9, #0          // r9 = j (contador columnas principales)

fase2_all_bucle_j:
    CMP     r9, r5          // comparar j con NUM_FILAS (solo primeras 9 columnas)
    BGE     fase2_all_fin_j // si j >= NUM_FILAS, terminar bucle j

    # Calcular dirección: cuadricula[i][j]
    MLA     r10, r8, r6, r9 // r10 = i * NUM_COLUMNAS + j
    MOV     r10, r10, LSL #1 // r10 *= 2 (sizeof CELDA)
    ADD     r10, r4, r10    // r10 = dirección de cuadricula[i][j]

    # Leer valor de la celda
    LDRH    r0, [r10]       // r0 = cuadricula[i][j]
    AND     r0, r0, #0x0F   // r0 = valor de la celda (bits 3-0)

    # Si celda está vacía (valor == 0), saltar propagación
    CMP     r0, #0
    BEQ     fase2_all_siguiente_j

    # ===== CÓDIGO INLINEADO DE CANDIDATOS_PROPAGAR =====
    # Variables actuales: r8=fila, r9=columna, r0=valor
    # Registros disponibles para reutilizar: r1,r2,r3
    
    # Calcular máscara para eliminar candidato: (1 << (7 + valor - 1))
    ADD     r1, r0, #6      // r1 = valor + 6 (7 + valor - 1)
    MOV     r2, #1          // r2 = 1
    MOV     r2, r2, LSL r1  // r2 = máscara para eliminar candidato

    # ===== PROPAGAR EN FILA (i=r8 fijo, j variable) =====
    MOV     r1, #0          // r1 = j_prop (contador columnas para propagación)

propagar_fila:
    CMP     r1, r5          // comparar j_prop con NUM_FILAS
    BGE     fin_propagar_fila

    # Calcular dirección: cuadricula[fila][j_prop]
    MLA     r3, r8, r6, r1  // r3 = fila * NUM_COLUMNAS + j_prop
    MOV     r3, r3, LSL #1  // r3 *= 2 (sizeof CELDA)
    ADD     r3, r4, r3      // r3 = dirección de cuadricula[fila][j_prop]

    # Eliminar candidato: celda |= máscara
    LDRH    r0, [r3]        // r0 = celda actual
    ORR     r0, r0, r2      // r0 |= máscara
    STRH    r0, [r3]        // guardar celda modificada

    ADD     r1, r1, #1      // j_prop++
    B       propagar_fila

fin_propagar_fila:
    # ===== PROPAGAR EN COLUMNA (j=r9 fijo, i variable) =====
    MOV     r1, #0          // r1 = i_prop (contador filas para propagación)

propagar_columna:
    CMP     r1, r5          // comparar i_prop con NUM_FILAS
    BGE     fin_propagar_columna

    # Calcular dirección: cuadricula[i_prop][columna]
    MLA     r3, r1, r6, r9  // r3 = i_prop * NUM_COLUMNAS + columna
    MOV     r3, r3, LSL #1  // r3 *= 2 (sizeof CELDA)
    ADD     r3, r4, r3      // r3 = dirección de cuadricula[i_prop][columna]

    # Eliminar candidato: celda |= máscara
    LDRH    r0, [r3]        // r0 = celda actual
    ORR     r0, r0, r2      // r0 |= máscara
    STRH    r0, [r3]        // guardar celda modificada

    ADD     r1, r1, #1      // i_prop++
    B       propagar_columna

fin_propagar_columna:
    # ===== PROPAGAR EN REGIÓN 3x3 =====
    # Calcular init_i = (fila / 3) * 3
    CMP     r8, #3
    MOVLT   r1, #0          // si fila < 3, init_i = 0
    MOVGE   r1, #3          // si fila >= 3, init_i = 3
    CMP     r8, #6
    MOVGE   r1, #6          // si fila >= 6, init_i = 6

    # Calcular init_j = (columna / 3) * 3
    CMP     r9, #3
    MOVLT   r3, #0          // si columna < 3, init_j = 0
    MOVGE   r3, #3          // si columna >= 3, init_j = 3
    CMP     r9, #6
    MOVGE   r3, #6          // si columna >= 6, init_j = 6

    # Guardar variables en pila usando frame pointer
    # fp-4 = init_i, fp-8 = init_j, fp-12 = end_i, fp-16 = end_j, fp-20 = máscara
    STR     r1, [fp, #-4]   // guardar init_i
    STR     r3, [fp, #-8]   // guardar init_j
    ADD     r0, r1, #3      // r0 = end_i
    STR     r0, [fp, #-12]  // guardar end_i
    ADD     r0, r3, #3      // r0 = end_j
    STR     r0, [fp, #-16]  // guardar end_j
    STR     r2, [fp, #-20]  // guardar máscara (r2 contiene la máscara)

    # Bucle región: for (i_reg = init_i; i_reg < end_i; i_reg++)
    MOV     r1, r1          // r1 = i_reg = init_i

all_bucle_region_i:
    LDR     r0, [fp, #-12]  // cargar end_i
    CMP     r1, r0          // comparar i_reg con end_i
    BGE     all_fin_region

    # Bucle región: for (j_reg = init_j; j_reg < end_j; j_reg++)
    LDR     r3, [fp, #-8]   // cargar init_j
    MOV     r3, r3          // r3 = j_reg = init_j

all_bucle_region_j:
    LDR     r0, [fp, #-16]  // cargar end_j
    CMP     r3, r0          // comparar j_reg con end_j
    BGE     all_fin_region_j

    # Calcular dirección: cuadricula[i_reg][j_reg]
    MLA     r0, r1, r6, r3  // r0 = i_reg * NUM_COLUMNAS + j_reg
    MOV     r0, r0, LSL #1  // r0 *= 2 (sizeof CELDA)
    ADD     r0, r4, r0      // r0 = dirección de cuadricula[i_reg][j_reg]

    # Eliminar candidato: celda |= máscara
    LDRH    r10, [r0]       // r10 = celda actual
    LDR     r2, [fp, #-20]  // cargar máscara
    ORR     r10, r10, r2    // r10 |= máscara
    STRH    r10, [r0]       // guardar celda modificada

    ADD     r3, r3, #1      // j_reg++
    B       all_bucle_region_j

all_fin_region_j:
    ADD     r1, r1, #1      // i_reg++
    B       all_bucle_region_i

all_fin_region:
    # ===== FIN CÓDIGO INLINEADO DE CANDIDATOS_PROPAGAR =====

fase2_all_siguiente_j:
    ADD     r9, r9, #1      // j++
    B       fase2_all_bucle_j

fase2_all_fin_j:
    ADD     r8, r8, #1      // i++
    B       fase2_all_bucle_i

fase2_all_fin:
    # Preparar valor de retorno
    MOV     r0, r7          // r0 = celdas_vacias (valor de retorno)

    # Restaurar pila y registros, luego retornar
    ADD     SP, SP, #8      // Liberar espacio de variables locales
    LDMIA   SP!, {r4-r10, fp, LR}
    BX      LR

#        END
