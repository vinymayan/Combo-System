# System Instructions - Combo System / PrismaUI

## Objetivo

Este projeto e um plugin SKSE para Skyrim que usa PrismaUI para HUD/overlay em HTML/CSS/JavaScript. Toda alteracao visual ou de comunicacao C++ <-> UI deve priorizar estabilidade em jogo, baixo custo de CPU e compatibilidade com as limitacoes do PrismaUI.

## PrismaUI: Limitacoes Obrigatorias

- O motor de UI e WebKit 615.1.18.100.1.
- JavaScript deve usar recursos ES2022 ou anteriores.
- Video nao e suportado. Use GIF apenas quando uma animacao bitmap for indispensavel.
- Audio nao e suportado. Sons devem ser tocados pelo SKSE/C++.
- WebGL nao e suportado. Nao use Three.js/WebGL/canvas WebGL.
- A UI e limitada a 60 FPS.
- A renderizacao e CPU-only no momento.
- Se TailwindCSS for usado, usar Tailwind v3.
- Evitar operacoes CSS pesadas em grande quantidade: `box-shadow`, `text-shadow`, `filter`, `backdrop-filter`, `blur`, gradientes complexos e multiplas camadas translucidas. Em HUDs de combate, trate sombras/filtros/blur como proibidos salvo justificativa clara.
- O evento JavaScript nativo `contextmenu` nao funciona corretamente. Para clique direito, sintetizar `contextmenu` a partir de `mousedown` com `event.button === 2`.
- Para bloquear teclas em inputs, combinar listeners de `keydown` e `beforeinput`.

## Performance Visual

- HUDs devem ser leves: poucos nos, poucas camadas, dimensoes estaveis e atualizacao incremental.
- Evitar re-renderizar a arvore inteira por frame. Atualize apenas propriedades que mudaram.
- Com Konva/canvas 2D:
  - usar poucas layers;
  - chamar `batchDraw()` em vez de `draw()` para atualizacoes frequentes;
  - parar `requestAnimationFrame` quando nao houver animacao visivel;
  - evitar sombras, filtros e blur;
  - manter elementos reaproveitados em vez de destruir/recriar nos.
- Nao fazer polling de alto custo na UI. O C++ deve empurrar estado somente quando houver mudanca relevante.
- Nao usar layouts ou efeitos que dependam de medicao constante de DOM.
- Nao usar animacoes CSS ou JS infinitas se a view estiver escondida ou sem dados uteis.
- Preferir transicoes simples de `transform`, `opacity`, largura/altura ou propriedades de canvas baratas.

## Comunicacao C++ -> JavaScript

- Para atualizacoes frequentes ou tempo real, usar `InteropCall`.
- `InteropCall` aceita apenas um argumento string e nao retorna valor. Empacote payloads simples com delimitador ou JSON curto.
- Para inicializacao, payloads complexos ou chamadas raras, usar `Invoke`.
- Registrar listeners JS com `RegisterJSListener` logo depois de criar a view e validar `PrismaUI`/`view`.
- Nao chamar UI se `PrismaUI` ou `view` forem invalidos.
- Para HUDs que precisam carregar cedo, criar/preload da view e esconder com `Hide`; mostrar com `Show` apenas quando houver dado real para exibir.
- Ao resetar estado de jogo (`NewGame`, `PostLoadGame`, reload), mandar payload de reset para a UI e esconder a view se ela nao deve aparecer.

## JavaScript / Frameworks Modernos

- Funcoes chamadas pelo SKSE devem ser registradas em `window` no carregamento do modulo, antes do framework montar componentes.
- Nao registrar funcoes `window` dentro de `onMount`, `useEffect` ou hooks equivalentes se o C++ puder chamar durante o DOM ready.
- Se usar framework moderno, guardar dados recebidos em store externo acessivel fora dos componentes. No Solid, um modulo de store/sinais exportado cumpre esse papel.
- Componentes devem ler do store e renderizar depois; a ponte SKSE nao deve depender do ciclo de vida visual do componente.
- Evitar `console.log`, `console.error` e afins em codigo entregue para o jogo. O PrismaUI em jogo nao oferece console util e logs frequentes custam performance.
- Evitar `try/catch` em hot paths JS. Validar payload antes de parsear/aplicar.
- Limpar listeners, animation frames e recursos no cleanup da aplicacao.

## UI/UX Para HUD De Combate

- Primeira tela deve ser a experiencia real, nao uma landing page.
- O HUD deve permanecer invisivel ate existir informacao util.
- Texto deve ter dimensoes estaveis e nao pode causar layout shift.
- Evitar sombras. Para legibilidade, preferir stroke simples, cores contrastantes e fundos compactos.
- Nao usar blur/bokeh/orbs/gradientes decorativos pesados.
- Barras de progresso devem atualizar por interpolacao leve e obedecer dados vindos do C++.
- Nao duplicar estado autoritativo na UI. A logica de combo/tier/expiracao fica no C++; a UI so apresenta e suaviza visualmente.

## Padroes C++ PrismaUI

- Guardar `PrismaView` persistente e checar validade antes de uso.
- Criar a view uma vez, reaproveitar com `Show`/`Hide`.
- Nao destruir/recriar view em eventos frequentes.
- Para payloads frequentes, preferir strings curtas.
- Nao fazer chamadas PrismaUI desnecessarias por frame; so enviar quando estado mudar.
- Registrar callbacks JS somente uma vez por view.
- Toda chamada vinda da UI para o C++ deve validar dados antes de tocar estado do jogo.

## Fontes De Referencia

- https://www.prismaui.dev/getting-started/introduction/
- https://www.prismaui.dev/guides/modern-frameworks/
- https://www.prismaui.dev/getting-started/limitations/
- `D:/a/GEMINI.md`
