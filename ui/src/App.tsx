import { onMount, onCleanup } from 'solid-js';
import Konva from 'konva';

function App() {
    let containerRef: HTMLDivElement | undefined;

    onMount(() => {
        if (!containerRef) return;

        const stage = new Konva.Stage({
            container: containerRef,
            width: window.innerWidth,
            height: window.innerHeight,
        });

        const layer = new Konva.Layer();
        stage.add(layer);

        const posX = window.innerWidth - 260;
        const posY = window.innerHeight - 220;

        // Texto Principal: Exibe a LETRA do Rank (F, E, D, C, B, A, S, SS, SSS, Z)
        const hitText = new Konva.Text({
            x: posX,
            y: posY,
            text: '',
            fontSize: 72,
            fontFamily: 'Impact, Arial Black, sans-serif',
            fill: '#ffffff',
            stroke: '#000000',
            strokeWidth: 4,
            align: 'center',
            width: 200,
        });

        // Subtexto: Exibe dinamicamente a quantidade de golpes (ex: "15 HITS")
        const labelText = new Konva.Text({
            x: posX,
            y: posY + 75,
            text: '',
            fontSize: 18,
            fontFamily: 'Arial, sans-serif',
            fill: '#ffaa00',
            fontStyle: 'bold',
            align: 'center',
            width: 200,
            letterSpacing: 2,
        });

        // Barra de progresso Background
        const barBg = new Konva.Rect({
            x: posX,
            y: posY + 105,
            width: 200,
            height: 12,
            fill: '#1a1a1a',
            stroke: '#333333',
            strokeWidth: 2,
            cornerRadius: 6,
        });

        // Barra de progresso Fill (Agora atua como o decaimento visual do timer)
        const barFill = new Konva.Rect({
            x: posX + 2,
            y: posY + 107,
            width: 0,
            height: 8,
            fillLinearGradientStartPoint: { x: 0, y: 0 },
            fillLinearGradientEndPoint: { x: 200, y: 0 },
            fillLinearGradientColorStops: [0, '#e65c00', 1, '#f9d423'],
            cornerRadius: 4,
        });

        layer.add(hitText);
        layer.add(labelText);
        layer.add(barBg);
        layer.add(barFill);
        layer.draw();

        // Variáveis internas de controle visual
        let visualTimeRemaining = 0;
        let isTimerPaused = false;
        let lastTime = performance.now();
        let animationFrameId: number;

        let prevHitValue = 0;
        let currentHitValue = 0;
        let currentComboValue = 0;

        const getStyleRank = (combo: number): string => {
            if (combo >= 90) return 'Z';
            if (combo >= 80) return 'SSS';
            if (combo >= 70) return 'SS';
            if (combo >= 60) return 'S';
            if (combo >= 50) return 'A';
            if (combo >= 40) return 'B';
            if (combo >= 30) return 'C';
            if (combo >= 20) return 'D';
            if (combo >= 10) return 'E';
            return 'F';
        };

        // Loop de Render para Animações Orgânicas e Esvaziamento Sincronizado da Barra
        const updateLoop = () => {
            const now = performance.now();
            const deltaTime = (now - lastTime) / 1000;
            lastTime = now;

            if (!isTimerPaused && visualTimeRemaining > 0) {
                visualTimeRemaining -= deltaTime;
                if (visualTimeRemaining < 0) visualTimeRemaining = 0;

                // ALTERAÇÃO 1: A própria barra do combo reduz linearmente de acordo com o tempo restante
                const maxComboWidth = 196;
                const baseRatio = Math.min(currentComboValue / 100, 1);
                const timeRatio = visualTimeRemaining / 20.0; // Proporção do tempo restante (20s max)

                // Combina o valor atual do combo com o escoamento síncrono do tempo
                barFill.width(maxComboWidth * baseRatio * timeRatio);

                // Efeito de oscilação dinâmica das chamas (Flicker Effect)
                if (currentHitValue > 0 && currentComboValue >= 30) {
                    const intensity = Math.min(currentComboValue / 100, 1);
                    const baseBlur = currentComboValue <= 59 ? 8 : (currentComboValue <= 89 ? 16 : 26);
                    const flicker = Math.sin(now / 35) * (4 * intensity) + Math.cos(now / 20) * (2 * intensity);

                    hitText.shadowBlur(baseBlur + flicker);

                    if (currentComboValue >= 90) {
                        const heatPulse = 1.0 + Math.sin(now / 40) * 0.03;
                        hitText.scale({ x: heatPulse, y: heatPulse });
                    }
                }

                layer.batchDraw();
            }

            animationFrameId = requestAnimationFrame(updateLoop);
        };
        animationFrameId = requestAnimationFrame(updateLoop);

        // Registro executado no InteropCall vindo do C++
        (window as any).updateComboMeter = (payload: string) => {
            if (!payload) return;

            const parts = payload.split('|');
            if (parts.length < 2) return;

            const hitValue = parseInt(parts[0], 10) || 0;
            const comboValue = parseInt(parts[1], 10) || 0;

            currentHitValue = hitValue;
            currentComboValue = comboValue;

            // Sempre que a contagem numérica de hits aumenta, resetamos o cronômetro visual para 20s
            if (hitValue > prevHitValue) {
                visualTimeRemaining = 20.0;
            } else if (hitValue === 0) {
                visualTimeRemaining = 0;
            }

            prevHitValue = hitValue;

            // Se o C++ expirou o combo (enviou 0|0), limpa totalmente os elementos da interface
            if (hitValue === 0) {
                hitText.text('');
                labelText.text('');
                barFill.width(0);
                hitText.shadowEnabled(false);
                layer.batchDraw();
                return;
            }

            const rankLetter = getStyleRank(comboValue);
            hitText.text(rankLetter);
            labelText.text(`${hitValue} HITS`);

            // Evolução progressiva de tamanho por intensidade do Rank
            const intensity = Math.min(comboValue / 100, 1);
            const newFontSize = 72 + (38 * intensity);
            hitText.fontSize(newFontSize);
            hitText.y(posY - (newFontSize - 72) * 0.5);

            // Configuração estética dos patamares de Rank (Cores e Filtros)
            if (comboValue < 30) {
                hitText.fill('#ffffff');
                hitText.stroke('#000000');
                hitText.strokeWidth(4);
                hitText.shadowEnabled(false);
                labelText.fill('#ffaa00');
            } else if (comboValue < 60) {
                hitText.fill('#ffea00');
                hitText.stroke('#1a1a1a');
                hitText.strokeWidth(4);
                hitText.shadowColor('#ff6600');
                hitText.shadowOpacity(0.75);
                hitText.shadowOffset({ x: 0, y: -2 });
                hitText.shadowEnabled(true);
                labelText.fill('#ffea00');
            } else if (comboValue < 90) {
                hitText.fill('#ff6600');
                hitText.stroke('#050505');
                hitText.strokeWidth(5);
                hitText.shadowColor('#ff1a00');
                hitText.shadowOpacity(0.85);
                hitText.shadowOffset({ x: 0, y: -4 });
                hitText.shadowEnabled(true);
                labelText.fill('#ff3300');
            } else {
                hitText.fill('#ff0022');
                hitText.stroke('#ffd700');
                hitText.strokeWidth(5);
                hitText.shadowColor('#ff0000');
                hitText.shadowOpacity(0.95);
                hitText.shadowOffset({ x: 0, y: -5 });
                hitText.shadowEnabled(true);
                labelText.fill('#ffd700');
            }

            // Impacto físico instantâneo ("Slams")
            const pulseScale = 1.12 + (0.08 * intensity);
            hitText.scale({ x: pulseScale, y: pulseScale });
            layer.batchDraw();

            setTimeout(() => {
                if (currentComboValue < 90) {
                    hitText.scale({ x: 1, y: 1 });
                }
                layer.batchDraw();
            }, 60);
        };

        (window as any).setComboTimerPaused = (payload: string) => {
            isTimerPaused = (payload === "true");
            lastTime = performance.now();
        };

        const handleResize = () => {
            stage.width(window.innerWidth);
            stage.height(window.innerHeight);
            const newX = window.innerWidth - 260;
            const newY = window.innerHeight - 220;

            const intensity = Math.min(currentComboValue / 100, 1);
            const currentFontSize = 72 + (38 * intensity);

            hitText.x(newX);
            hitText.y(newY - (currentFontSize - 72) * 0.5);
            labelText.x(newX);
            barBg.x(newX);
            barFill.x(newX + 2);
            layer.batchDraw();
        };

        window.addEventListener('resize', handleResize);

        onCleanup(() => {
            window.removeEventListener('resize', handleResize);
            cancelAnimationFrame(animationFrameId);
            delete (window as any).updateComboMeter;
            delete (window as any).setComboTimerPaused;
            stage.destroy();
        });
    });

    return (
        <div
            ref={containerRef}
            style={{
                position: 'absolute',
                top: 0,
                left: 0,
                width: '100vw',
                height: '100vh',
                'pointer-events': 'none',
                overflow: 'hidden',
            }}
        />
    );
}

export default App;