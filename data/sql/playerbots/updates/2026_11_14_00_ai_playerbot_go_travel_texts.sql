DELETE FROM ai_playerbot_texts WHERE name IN (
    'go_travel_start',
    'go_travel_unknown',
    'go_travel_arrived',
    'go_travel_cancelled',
    'go_travel_not_traveling'
);

DELETE FROM ai_playerbot_texts_chance WHERE name IN (
    'go_travel_start',
    'go_travel_unknown',
    'go_travel_arrived',
    'go_travel_cancelled',
    'go_travel_not_traveling'
);

INSERT INTO ai_playerbot_texts
    (id, name, text, say_type, reply_type, text_loc1, text_loc2, text_loc3, text_loc4, text_loc5, text_loc6, text_loc7, text_loc8)
VALUES
    (1948, 'go_travel_start', 'Traveling to %destination', 0, 0, '%destination(으)로 이동합니다', 'Je pars pour %destination', 'Ich reise nach %destination', '正在前往%destination', '正在前往%destination', 'Viajando a %destination', 'Viajando a %destination', 'Отправляюсь: %destination'),
    (1949, 'go_travel_unknown', 'I don''t know where %destination is', 0, 0, '%destination 위치를 모르겠습니다', 'Je ne sais pas où se trouve %destination', 'Ich weiß nicht, wo %destination ist', '我不知道%destination在哪里', '我不知道%destination在哪裡', 'No sé dónde está %destination', 'No sé dónde está %destination', 'Я не знаю, где находится %destination'),
    (1950, 'go_travel_arrived', 'Arrived at %destination', 0, 0, '%destination에 도착했습니다', 'Arrivé à %destination', 'Angekommen: %destination', '已到达%destination', '已到達%destination', 'He llegado a %destination', 'Ya llegué a %destination', 'Прибыл: %destination'),
    (1951, 'go_travel_cancelled', 'Travel cancelled', 0, 0, '이동을 취소했습니다', 'Voyage annulé', 'Reise abgebrochen', '已取消旅行', '已取消旅行', 'Viaje cancelado', 'Viaje cancelado', 'Путешествие отменено'),
    (1952, 'go_travel_not_traveling', 'I''m not traveling anywhere', 0, 0, '지금은 이동 중이 아닙니다', 'Je ne voyage nulle part', 'Ich bin gerade nicht unterwegs', '我现在没有在旅行', '我現在沒有在旅行', 'No estoy viajando a ningún sitio', 'No estoy viajando a ningún lado', 'Я сейчас никуда не направляюсь');

INSERT INTO ai_playerbot_texts_chance (name, probability) VALUES
    ('go_travel_start', 100),
    ('go_travel_unknown', 100),
    ('go_travel_arrived', 100),
    ('go_travel_cancelled', 100),
    ('go_travel_not_traveling', 100);
