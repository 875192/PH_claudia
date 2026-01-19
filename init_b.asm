@ Asignatura: Proyecto hardware
@ Fecha: 09/10/2025
@ Autores: Francisco Jose Martinez, Claudia Mateo, Nayara Gomez
@ Descripcion: Implementa rutinas ARM para actualizar y propagar candidatos en
@              tableros Sudoku 9x9. Contiene candidatos_propagar_arm,
@              candidatos_actualizar_arm_c, candidatos_actualizar_arm y
@              candidatos_actualizar_all.

.text

.extern celda_eliminar_candidato
.extern celda_leer_valor
.extern candidatos_propagar_c
.global candidatos_propagar_arm
.global candidatos_actualizar_arm_c
.global candidatos_actualizar_arm
.global candidatos_actualizar_all

@ =============================================================================
@ FUNCIÓN: candidatos_propagar_arm
@ DESCRIPCIÓN: Propaga la eliminación de candidatos para una celda con valor.
@              Elimina el valor de la celda de todos los candidatos de su fila,
@              columna y región 3x3 correspondiente usando subfunciones ARM.
@ PARÁMETROS:
@   r0 - Puntero a la cuadrícula (CELDA cuadricula[9][16])
@   r1 - Número de fila (0-8)
@   r2 - Número de columna (0-8)
@ REGISTROS UTILIZADOS:
@   r4  - Puntero base a la cuadrícula (preservado durante toda la función)
@   r5  - Desplazamiento para crear máscara (valor + 6)
@   r6  - Contenido de celda actual (16 bits)
@   r7  - Máscara de eliminación (1 << (valor+6)), compartida con subfunciones
@   r8  - Valor extraído de la celda (bits 0-3)
@   r9  - Número de fila de la celda origen (preservado para subfunciones)
@   r10 - Número de columna de la celda origen (preservado para subfunciones)
@ VALOR DEVUELTO: Ninguno (void)
@ NOTA: Utiliza BL para llamar a recorrer_fila, recorrer_columna y recorrer_region
@ =============================================================================

candidatos_propagar_arm:
        MOV     IP, SP                        
        STMDB   SP!, {r4-r10,FP,IP,LR,PC}     
        SUB     FP, IP, #4                   

        MOV     r4, r0         
        MOV     r9, r1       
        MOV     r10, r2        
        ADD     r3, r10, r9, LSL #4  
        LSL     r3, r3, #1           
        LDRH    r6, [r4, r3]   
        AND     r8, r6, #0xF     
        ADD     r5, r8, #6     
        MOV     r7, #1          
        LSL     r7, r7, r5       
        ORR     r6, r6, r7       
        STRH    r6, [r4, r3]     

        BL      recorrer_fila
        BL      recorrer_columna
        BL      recorrer_region

        LDMDB   FP, {r4-r10,FP,SP,PC}          

recorrer_fila:
        MOV     r3, #0          

        bucle_fila:
                CMP     r3, #9
                BGE     fin_fila
                CMP     r3, r10       
                BEQ     skip_fila_mem   

                ADD     r1, r3, r9, LSL #4     
                LSL     r1, r1, #1            
                LDRH    r2, [r4, r1]    
                ORR     r2, r2, r7       
                STRH    r2, [r4, r1]     

                skip_fila_mem:
                        ADD     r3, r3, #1
                        B       bucle_fila

                fin_fila:
                        BX      lr

recorrer_columna:
        MOV     r3, #0          

        bucle_columna:
                CMP     r3, #9
                BGE     fin_columna
                CMP     r3, r9       
                BEQ     skip_col_mem    

                ADD     r1, r10, r3, LSL #4    
                LSL     r1, r1, #1           
                LDRH    r2, [r4, r1]     
                ORR     r2, r2, r7       
                STRH    r2, [r4, r1]   

                skip_col_mem:
                        ADD     r3, r3, #1
                        B       bucle_columna

                fin_columna:
                        BX      lr

recorrer_region:
        LDR     r1, =init_region
        LDRB    r2, [r1, r9]       
        LDRB    r3, [r1, r10]      
        
        ADD     r5, r2, #3         
        ADD     r6, r3, #3     
        MOV     r1, r2            

        bucle_region_i:
                CMP     r1, r5
                BGE     fin_region
                MOV     r2, r3        

        bucle_region_j:
                CMP     r2, r6
                BGE     fin_region_j
                CMP     r1, r9
                BNE     region_diferente
                CMP     r2, r10
                BEQ     salto_region_mem

                region_diferente:
                        ADD     r0, r2, r1, LSL #4    
                        LSL     r0, r0, #1         
                        LDRH    r8, [r4, r0]   
                        ORR     r8, r8, r7     
                        STRH    r8, [r4, r0]

                salto_region_mem:
                        ADD     r2, r2, #1
                        B       bucle_region_j

                fin_region_j:
                        ADD     r1, r1, #1
                        B       bucle_region_i

                fin_region:
                        BX      lr

@ =============================================================================
@ FUNCIÓN: candidatos_actualizar_arm_c
@ DESCRIPCIÓN: Actualiza candidatos de toda la cuadrícula combinando limpieza
@              en ARM y propagación llamando a candidatos_propagar_c (función C).
@              Primero limpia todos los candidatos, luego procesa cada celda.
@ PARÁMETROS:
@   r0 - Puntero a la cuadrícula (CELDA cuadricula[9][16])
@ REGISTROS UTILIZADOS:
@   r4  - Puntero base a la cuadrícula (preservado durante toda la función)
@   r5  - Contador de celdas vacías (valor de retorno final)
@   r6  - Índice de fila actual (0-8) durante limpieza y procesamiento
@   r7  - Índice de columna actual (0-8) durante limpieza y procesamiento
@   r8  - Máscara de limpieza inicial (0x7F) para preservar bits 0-6
@   r9  - Offset temporal para calcular direcciones de memoria
@   r10 - Contenido de celda leído para verificar si tiene valor
@ LLAMADAS EXTERNAS:
@   - celda_leer_valor: extrae el valor de una celda
@   - candidatos_propagar_c: propaga eliminación de candidatos (función C)
@ VALOR DEVUELTO: r0 - Número total de celdas vacías encontradas
@ =============================================================================

candidatos_actualizar_arm_c:
        MOV     IP, SP                         
        STMDB   SP!, {r4-r10,FP,IP,LR,PC}      
        SUB     FP, IP, #4                      

        MOV     r4, r0         
        MOV     r5, #0       
        MOV     r6, #0         
        MOV     r8, #0x7F     

        limpiar_filas:
                CMP     r6, #9
                BGE     fin_limpiar_filas
                MOV     r7, #0        

        limpiar_columnas:
                CMP     r7, #9
                BGE     limpiar_siguiente_fila
                
                ADD     r9, r7, r6, LSL #4     
                LSL     r9, r9, #1

                LDRH    r1, [r4, r9]
                AND     r1, r1, r8
                STRH    r1, [r4, r9]

                ADD     r7, r7, #1      
                B       limpiar_columnas

                limpiar_siguiente_fila:
                        ADD     r6, r6, #1
                        B       limpiar_filas

                fin_limpiar_filas:
                        MOV     r6, #0          

        bucle_filas:
                CMP     r6, #9
                BGE     fin_bucle_filas
                MOV     r7, #0        

        bucle_columnas_principal:
                CMP     r7, #9
                BGE     fin_bucle_columnas_principal

                ADD     r9, r7, r6, LSL #4
                LSL     r9, r9, #1
                LDRH    r10, [r4, r9]         
                MOV     r0, r10              
                BL      celda_leer_valor

                CMP     r0, #0
                ADDEQ   r5, r5, #1      
                MOVNE   r0, r4          
                MOVNE   r1, r6         
                MOVNE   r2, r7       
                BLNE    candidatos_propagar_c

                ADD     r7, r7, #1  
                B       bucle_columnas_principal

        fin_bucle_columnas_principal:
                ADD     r6, r6, #1
                B       bucle_filas

        fin_bucle_filas:
                MOV     r0, r5          
                LDMDB   FP, {r4-r10,FP,SP,PC}          

@ =============================================================================
@ FUNCIÓN: candidatos_actualizar_arm
@ DESCRIPCIÓN: Actualiza candidatos de toda la cuadrícula completamente en ARM.
@              Limpia candidatos y propaga eliminaciones llamando a 
@              candidatos_propagar_arm para cada celda con valor.
@ PARÁMETROS:
@   r0 - Puntero a la cuadrícula (CELDA cuadricula[9][16])
@ REGISTROS UTILIZADOS:
@   r4  - Puntero base a la cuadrícula (preservado durante toda la función)
@   r5  - Contador de celdas vacías (valor de retorno final)
@   r6  - Índice de fila actual (0-8) durante limpieza y procesamiento
@   r7  - Índice de columna actual (0-8) durante limpieza y procesamiento
@   r8  - Máscara de limpieza inicial (0x7F) para preservar bits 0-6
@   r9  - Offset temporal para calcular direcciones de memoria
@   r10 - Contenido de celda leído para verificar si tiene valor
@ LLAMADAS EXTERNAS:
@   - celda_leer_valor: extrae el valor de una celda
@   - candidatos_propagar_arm: propaga eliminación de candidatos (función ARM)
@ VALOR DEVUELTO: r0 - Número total de celdas vacías encontradas
@ NOTA: Versión totalmente ARM que llama a candidatos_propagar_arm
@ =============================================================================
candidatos_actualizar_arm:
        MOV     IP, SP                        
        STMDB   SP!, {r4-r10,FP,IP,LR,PC}     
        SUB     FP, IP, #4                    

        MOV     r4, r0         
        MOV     r5, #0       
        MOV     r6, #0         
        MOV     r8, #0x7F      

        limpiar_filas_arm:
                CMP     r6, #9
                BGE     fin_limpiar_filas_arm
                MOV     r7, #0        

        limpiar_columnas_arm:
                CMP     r7, #9
                BGE     limpiar_siguiente_fila_arm
                ADD     r9, r7, r6, LSL #4     
                LSL     r9, r9, #1            
                LDRH    r1, [r4, r9]          
                AND     r1, r1, r8            
                STRH    r1, [r4, r9]          
                ADD     r7, r7, #1      
                B       limpiar_columnas_arm

        limpiar_siguiente_fila_arm:
                ADD     r6, r6, #1
                B       limpiar_filas_arm

                fin_limpiar_filas_arm:
                        MOV     r6, #0          

        bucle_filas_arm:
                CMP     r6, #9
                BGE     fin_bucle_filas_arm
                MOV     r7, #0        

        bucle_columnas_principal_arm:
                CMP     r7, #9
                BGE     fin_bucle_columnas_principal_arm
                
                ADD     r9, r7, r6, LSL #4     
                LSL     r9, r9, #1           
                LDRH    r10, [r4, r9]        
                MOV     r0, r10              
                BL      celda_leer_valor
                
                CMP     r0, #0
                ADDEQ   r5, r5, #1     
                MOVNE   r0, r4        
                MOVNE   r1, r6        
                MOVNE   r2, r7         
                BLNE    candidatos_propagar_arm

                ADD     r7, r7, #1      
                B       bucle_columnas_principal_arm

                fin_bucle_columnas_principal_arm:
                        ADD     r6, r6, #1
                        B       bucle_filas_arm

                fin_bucle_filas_arm:
                        MOV     r0, r5          
                        LDMDB   FP, {r4-r10,FP,SP,PC}          

@ =============================================================================
@ FUNCIÓN: candidatos_actualizar_all
@ DESCRIPCIÓN: Implementación completa en ARM assembly que actualiza candidatos
@              con propagación inline. Limpia candidatos, cuenta celdas vacías
@              y propaga eliminaciones sin llamadas a funciones externas.
@ PARÁMETROS:
@   r0 - Puntero a la cuadrícula (CELDA cuadricula[9][16])
@ REGISTROS UTILIZADOS:
@   r4  - Puntero base a la cuadrícula (preservado durante toda la función)
@   r5  - Contador de celdas vacías (valor de retorno final)
@   r6  - Índice de fila principal / fila temporal en región
@   r7  - Índice de columna principal / columna temporal en región
@   r8  - Offset memoria / máscara eliminación / registro temporal multiuso
@   r9  - Máscara limpieza (0x7F) / valores recuperados desde pila
@   r10 - Contenido celda / valores recuperados desde pila
@ STACK UTILIZADO:
@   - [SP, SP+4]: Coordenadas originales (fila, columna) durante propagación
@   - [SP, SP+12]: Límites y coordenadas región durante recorrido 3x3
@ LLAMADAS EXTERNAS:
@   - celda_leer_valor: única función externa, para extraer valor de celda
@ VALOR DEVUELTO: r0 - Número total de celdas vacías encontradas
@ NOTA: Versión más optimizada con propagación inline sin llamadas a subfunciones
@ =============================================================================
candidatos_actualizar_all:
        MOV     IP, SP                        
        STMDB   SP!, {r4-r10,FP,IP,LR,PC}     
        SUB     FP, IP, #4                   

        MOV     r4, r0         
        MOV     r5, #0        
        MOV     r6, #0      
        MOV     r9, #0x7F     

        limpiar_filas_all:
                CMP     r6, #9
                BGE     fin_limpiar_filas_all
                MOV     r7, #0        

        limpiar_columnas_all:
                CMP     r7, #9
                BGE     limpiar_siguiente_fila_all
                
                ADD     r8, r7, r6, LSL #4   
                LSL     r8, r8, #1           
                LDRH    r1, [r4, r8]          
                AND     r1, r1, r9             
                STRH    r1, [r4, r8]          
                ADD     r7, r7, #1      
                B       limpiar_columnas_all

                limpiar_siguiente_fila_all:
                        ADD     r6, r6, #1
                        B       limpiar_filas_all

                fin_limpiar_filas_all:
                        MOV     r6, #0         

        bucle_filas_all:
                CMP     r6, #9
                BGE     fin_bucle_filas_all

                MOV     r7, #0        

        bucle_columnas_principal_all:
                CMP     r7, #9
                BGE     fin_bucle_columnas_principal_all

                ADD     r8, r7, r6, LSL #4   
                LSL     r8, r8, #1            
                LDRH    r10, [r4, r8]         
                MOV     r0, r10                
                BL      celda_leer_valor

                CMP     r0, #0
                ADDEQ   r5, r5, #1            
                BEQ     salto_propagar_all     

                @ Código inline
                STMDB   SP!, {r6, r7}        
                ADD     r3, r7, r6, LSL #4  
                LSL     r3, r3, #1           
                LDRH    r0, [r4, r3]     
                AND     r1, r0, #0xF          
                ADD     r2, r1, #6            
                MOV     r8, #1          
                LSL     r8, r8, r2            
                ORR     r0, r0, r8       
                STRH    r0, [r4, r3]     
                MOV     r3, #0        

                bucle_fila_all:
                        CMP     r3, #9
                        BGE     fin_fila_all

                        LDR     r9, [SP, #4]        
                        CMP     r3, r9                
                        BEQ     salto_fila           

                        LDR     r10, [SP]           
                        ADD     r1, r3, r10, LSL #4   
                        LSL     r1, r1, #1           
                        LDRH    r2, [r4, r1]          
                        ORR     r2, r2, r8            
                        STRH    r2, [r4, r1]          

                salto_fila:
                        ADD     r3, r3, #1      
                        B       bucle_fila_all        

                fin_fila_all:
                        MOV     r3, #0          

                bucle_columna_all:
                        CMP     r3, #9
                        BGE     fin_columna_all

                        LDR     r10, [SP]             
                        CMP     r3, r10               
                        BEQ     salto_columna    

                        LDR     r9, [SP, #4]          
                        ADD     r1, r9, r3, LSL #4   
                        LSL     r1, r1, #1         

                        LDRH    r2, [r4, r1]        
                        ORR     r2, r2, r8            
                        STRH    r2, [r4, r1]          

                salto_columna:
                        ADD     r3, r3, #1      
                        B       bucle_columna_all     

                fin_columna_all:
                        LDR     r1, =init_region
                        LDR     r10, [SP]            
                        LDR     r9, [SP, #4]        
                        LDRB    r2, [r1, r10]         
                        LDRB    r3, [r1, r9]          
                        
                        ADD     r0, r2, #3           
                        ADD     r1, r3, #3          
                        STMDB   SP!, {r0, r1, r2, r3} 
                        
                        MOV     r6, r2                

                bucle_region_i_all:
                        LDR     r0, [SP]              
                        CMP     r6, r0            
                        BGE     fin_region_all
                        LDR     r7, [SP, #12]         
                        MOV     r2, r7               

                bucle_region_j_all:
                        LDR     r1, [SP, #4]          
                        CMP     r2, r1             
                        BGE     fin_region_j_all

                        LDR     r10, [SP, #16]       
                        LDR     r9, [SP, #20]         
                        
                        CMP     r6, r10               
                        BNE     region_diferente_all
                        CMP     r2, r9                
                        BEQ     salto_region 

                region_diferente_all:
                        ADD     r3, r2, r6, LSL #4   
                        LSL     r3, r3, #1            
                        
                        LDRH    r0, [r4, r3]     
                        ORR     r0, r0, r8            
                        STRH    r0, [r4, r3]          

                salto_region:
                        ADD     r2, r2, #1            
                        B       bucle_region_j_all    

                fin_region_j_all:
                        ADD     r6, r6, #1            
                        B       bucle_region_i_all    

                fin_region_all:
                        ADD     SP, SP, #16           
                        ADD     SP, SP, #8            

                salto_propagar_all:        
                        ADD     r7, r7, #1     
                        B       bucle_columnas_principal_all

                fin_bucle_columnas_principal_all:
                        ADD     r6, r6, #1      
                        B       bucle_filas_all       

        fin_bucle_filas_all:
                MOV     r0, r5          
                LDMDB   FP, {r4-r10,FP,SP,PC}
                
    .data
init_region:
     .byte     0,0,0,3,3,3,6,6,6

.end

